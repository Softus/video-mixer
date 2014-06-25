/*
 * Copyright (C) 2013-2014 Irkutsk Diagnostic Center.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define QT_NO_EMIT
#include "typedetect.h"
#include "product.h"
#include <gio/gio.h>

#include <QDebug>
#include <QImage>
#include <QSettings>
#include <QUrl>

#include <QGlib/Signal>
#include <QGst/Buffer>
#include <QGst/Caps>
#include <QGst/Event>
#include <QGst/ElementFactory>
#include <QGst/Fourcc>
#include <QGst/Pipeline>
#include <QGst/Structure>
#include <QGst/ClockTime>

bool SetFileExtAttribute(const QString& filePath, const QString& name, const QString& value)
{
    auto encodedValue = QUrl::toPercentEncoding(value);

    bool ret = false;
    auto file = g_file_new_for_path(filePath.toLocal8Bit());
    if (file)
    {
        GError* err = nullptr;
        QString attr("xattr::" PRODUCT_NAMESPACE ".");
        attr.append(name);
        ret = g_file_set_attribute_string (file, attr.toLocal8Bit(),
            encodedValue.data(), G_FILE_QUERY_INFO_NONE, nullptr, &err);

        if (err)
        {
#ifdef Q_OS_WIN
            // Backup route for windows
            //
            if (err->domain == G_IO_ERROR && err->code == G_IO_ERROR_NOT_SUPPORTED)
            {
                QSettings settings(filePath + ":" + PRODUCT_NAMESPACE, QSettings::IniFormat);
                settings.setValue(name, encodedValue);
                settings.sync();
                ret = (settings.status() == QSettings::NoError);
            }
#else
            qDebug() << QString::fromLocal8Bit(err->message);
#endif
            g_error_free(err);
        }

        g_object_unref(file);
    }
    return ret;
}

QString GetFileExtAttribute(const QString& filePath, const QString& name)
{
    QByteArray encodedValue;
    auto file = g_file_new_for_path(filePath.toLocal8Bit());
    if (file)
    {
        QString attr("xattr::" PRODUCT_NAMESPACE ".");
        attr.append(name);

        GError* err = nullptr;
        auto info = g_file_query_info(file, attr.toLocal8Bit(),
            G_FILE_QUERY_INFO_NONE, nullptr, &err);

        if (info)
        {
            auto value = g_file_info_get_attribute_string(info, attr.toLocal8Bit());
            if (value)
            {
                encodedValue.append(value);
            }
            g_object_unref(info);
        }

#ifdef Q_OS_WIN
        if (encodedValue.isNull())
        {
            // Backup route for windows
            //
            QSettings settings(filePath + ":" + PRODUCT_NAMESPACE, QSettings::IniFormat);
            encodedValue = settings.value(name).toByteArray();
        }
#endif
        if (err)
        {
            qDebug() << QString::fromLocal8Bit(err->message);
            g_error_free(err);
        }

        g_object_unref(file);
    }
    return QUrl::fromPercentEncoding(encodedValue);
}

QString TypeDetect(const QString& filePath)
{
    QGst::State   state;
    QGst::CapsPtr caps;
    auto pipeline = QGst::Pipeline::create("typedetect");
    auto source   = QGst::ElementFactory::make("filesrc", "source");
    auto typefind = QGst::ElementFactory::make("typefind", "typefind");
    auto fakesink = QGst::ElementFactory::make("fakesink", "fakesink");

    if (pipeline && source && typefind && fakesink)
    {
        pipeline->add(source, typefind, fakesink);
        QGst::Element::linkMany(source, typefind, fakesink);

        source->setProperty("location", filePath);
        pipeline->setState(QGst::StatePaused);
        auto timeout = QGst::ClockTime::fromSeconds(10);
        if (QGst::StateChangeSuccess == pipeline->getState(&state, nullptr, timeout))
        {
            auto prop = typefind->property("caps");
            if (prop)
            {
                caps = prop.get<QGst::CapsPtr>();
            }
        }
        pipeline->setState(QGst::StateNull);
        pipeline->getState(&state, nullptr, timeout);
    }

    if (caps)
    {
        auto str = caps->internalStructure(0);
        if (str)
        {
            return str->name();
        }
    }

    return "";
}

QImage ExtractRgbImage(const QGst::BufferPtr& buf, int width = 0)
{
    if (!buf)
    {
        return QImage();
    }

    auto structure = buf->caps()->internalStructure(0);
    auto imgWidth  = structure->value("width").toInt();
    auto imgHeight = structure->value("height").toInt();

    QImage img(buf->data(), imgWidth, imgHeight, QImage::Format_RGB888);

    // Must copy image bits, they will be unavailable after the pipeline stops
    //
    return width > 0? img.scaledToWidth(width): img.copy();
}

QImage ExtractImage(const QGst::BufferPtr& buf, int width = 0)
{
    QImage img;
    QGst::State   state;
    auto caps = buf->caps();
    auto structure = buf->caps()->internalStructure(0);
    if (structure->name() == "video/x-raw-rgb" && structure->value("bpp").toInt() == 24)
    {
        // Already good enought buffer
        //
        return ExtractRgbImage(buf, width);
    }

    auto pipeline = QGst::Pipeline::create("imgconvert");
    auto src   = QGst::ElementFactory::make("appsrc", "src");
    auto vaapi = structure->name() == "video/x-surface"?
         QGst::ElementFactory::make("vaapidownload", "vaapi"): QGst::ElementPtr();
    auto cvt   = QGst::ElementFactory::make("ffmpegcolorspace", "cvt");
    auto sink  = QGst::ElementFactory::make("appsink", "sink");

    if (pipeline && src && cvt && sink)
    {
        //qDebug() << caps->toString();
        pipeline->add(src, cvt, sink);
        if (vaapi)
        {
            pipeline->add(vaapi);
        }
        src->setProperty("caps", caps);
        sink->setProperty("caps", QGst::Caps::fromString("video/x-raw-rgb,bpp=24"));
        sink->setProperty("async", false);

        if (vaapi? QGst::Element::linkMany(src, vaapi, cvt, sink): QGst::Element::linkMany(src, cvt, sink))
        {
            pipeline->setState(QGst::StatePaused);
            auto timeout = QGst::ClockTime::fromMSecs(200);
            if (QGst::StateChangeSuccess == pipeline->getState(&state, nullptr, timeout))
            {
                QGlib::emit<void>(src, "push-buffer", buf);
                img = ExtractImage(QGlib::emit<QGst::BufferPtr>(sink, "pull-preroll"), width);
            }
            pipeline->setState(QGst::StateNull);
            pipeline->getState(&state, nullptr, timeout);
        }
    }

    return img;
}
