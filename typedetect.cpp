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

#include "typedetect.h"
#include "product.h"
#include <gio/gio.h>

#include <QSettings>
#include <QUrl>

#include <QGst/ElementFactory>
#include <QGst/Pipeline>
#include <QGst/Structure>

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

        auto info = g_file_query_info(file, attr.toLocal8Bit(),
            G_FILE_QUERY_INFO_NONE, nullptr, nullptr);

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
        if (QGst::StateChangeSuccess == pipeline->getState(&state, nullptr, -1))
        {
            auto prop = typefind->property("caps");
            if (prop)
            {
                caps = prop.get<QGst::CapsPtr>();
            }
        }
        pipeline->setState(QGst::StateNull);
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
