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

#include "videosourcedetails.h"
#include "../defaults.h"
#include <algorithm>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTextEdit>

// Video encoder
//
#include <QGlib/Error>
#include <QGlib/ParamSpec>
#include <QGlib/Value>
#include <QGst/Caps>
#include <QGst/ElementFactory>
#include <QGst/IntRange>
#include <QGst/Pad>
#include <QGst/Parse>
#include <QGst/Pipeline>
#include <QGst/PropertyProbe>
#include <QGst/Structure>
#include <gst/gst.h>
#include <gst/interfaces/tuner.h>

VideoSourceDetails::VideoSourceDetails(const QString& device, const QString& elmName, const QString& propName, QWidget *parent)
  : QDialog(parent)
  , checkFps(nullptr)
  , spinFps(nullptr)
{
    QSettings settings;
    settings.beginGroup("gst");
    auto layout = new QFormLayout();

    layout->addRow(tr("I&nput channel"), listChannels = new QComboBox());
    listChannels->addItem(tr("(default)"));
    connect(listChannels, SIGNAL(currentIndexChanged(int)), this, SLOT(inputChannelChanged(int)));
    layout->addRow(tr("Pixel &format"), listFormats = new QComboBox());
    connect(listFormats, SIGNAL(currentIndexChanged(int)), this, SLOT(formatChanged(int)));
    layout->addRow(tr("Frame &size"), listSizes = new QComboBox());
    layout->addRow(tr("Video &codec"), listVideoCodecs = new QComboBox());

    auto elm = QGst::ElementFactory::make("videorate");
    if (elm && elm->findProperty("max-rate"))
    {
        layout->addRow(checkFps = new QCheckBox(tr("&Limit rate")), spinFps = new QSpinBox());
        connect(checkFps, SIGNAL(toggled(bool)), spinFps, SLOT(setEnabled(bool)));
        checkFps->setChecked(settings.value("limit-video-fps", DEFAULT_LIMIT_VIDEO_FPS).toBool());

        spinFps->setRange(1, 200);
        spinFps->setValue(settings.value("video-max-fps", DEFAULT_VIDEO_MAX_FPS).toInt());
        spinFps->setSuffix(tr(" frames per second"));
        spinFps->setEnabled(checkFps->isChecked());
    }
    layout->addRow(tr("Video &bitrate"), spinBitrate = new QSpinBox());
    spinBitrate->setRange(0, 102400);
    spinBitrate->setSingleStep(100);
    spinBitrate->setSuffix(tr(" kbit per second"));
    spinBitrate->setValue(settings.value("bitrate", DEFAULT_VIDEOBITRATE).toInt());

    layout->addRow(nullptr, checkDeinterlace = new QCheckBox(tr("De&interlace")));
    checkDeinterlace->setChecked(settings.value("video-deinterlace").toBool());

    layout->addRow(tr("Video &muxer"), listVideoMuxers = new QComboBox());
    layout->addRow(tr("Im&age codec"), listImageCodecs = new QComboBox());
    layout->addRow(tr("RTP &payloader"), listRtpPayloaders = new QComboBox());

    // UDP streaming
    //
    textRtpClients = new QLineEdit(settings.value("rtp-clients").toString());
    layout->addRow(checkEnableRtp = new QCheckBox(tr("&RTP clients")), textRtpClients);
    connect(checkEnableRtp, SIGNAL(toggled(bool)), textRtpClients, SLOT(setEnabled(bool)));
    checkEnableRtp->setChecked(settings.value("enable-rtp").toBool());
    textRtpClients->setEnabled(checkEnableRtp->isChecked());

    // Http streaming
    //
    textHttpPushUrl = new QLineEdit(settings.value("http-push-url").toString());
    layout->addRow(checkEnableHttp = new QCheckBox(tr("&Http push URL")), textHttpPushUrl);
    connect(checkEnableHttp, SIGNAL(toggled(bool)), textHttpPushUrl, SLOT(setEnabled(bool)));
    checkEnableHttp->setChecked(settings.value("enable-http").toBool());
    textHttpPushUrl->setEnabled(checkEnableHttp->isChecked());

    setLayout(layout);

    // Refill the boxes every time the page is shown
    //
    auto selectedCodec = updateGstList("video-encoder", DEFAULT_VIDEO_ENCODER, GST_ELEMENT_FACTORY_TYPE_ENCODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO, listVideoCodecs);
    listVideoCodecs->insertItem(0, tr("(none)"));
    if (selectedCodec.isEmpty())
    {
        listVideoCodecs->setCurrentIndex(0);
    }
    auto selectedMuxer = updateGstList("video-muxer",   DEFAULT_VIDEO_MUXER,   GST_ELEMENT_FACTORY_TYPE_MUXER, listVideoMuxers);
    listVideoMuxers->insertItem(0, tr("(none)"));
    if (selectedMuxer.isEmpty())
    {
        listVideoMuxers->setCurrentIndex(0);
    }
    updateGstList("image-encoder", DEFAULT_IMAGE_ENCODER, GST_ELEMENT_FACTORY_TYPE_ENCODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE, listImageCodecs);
    updateGstList("rtp-payloader", DEFAULT_RTP_PAYLOADER, GST_ELEMENT_FACTORY_TYPE_PAYLOADER, listRtpPayloaders);

    updateDevice(device, elmName, propName);
}

QString VideoSourceDetails::updateGstList(const char* setting, const char* def, unsigned long long type, QComboBox* cb)
{
    QSettings settings;
    settings.beginGroup("gst");

    cb->clear();
    auto selectedCodec = settings.value(setting, def).toString();
    auto extra = settings.value(QString(setting)+"-extra").toBool() || qApp->keyboardModifiers().testFlag(Qt::ShiftModifier);
    auto elmList = gst_element_factory_list_get_elements(type, extra? GST_RANK_NONE: GST_RANK_SECONDARY);
    for (auto curr = elmList; curr; curr = curr->next)
    {
        auto factory = QGst::ElementFactoryPtr::wrap(GST_ELEMENT_FACTORY(curr->data), true);
        cb->addItem(factory->longName(), factory->name());
        if (selectedCodec == factory->name())
        {
            cb->setCurrentIndex(cb->count() - 1);
        }
    }

    gst_plugin_feature_list_free(elmList);
    return selectedCodec;
}

void VideoSourceDetails::updateDevice(const QString& device, const QString& elmName, const QString& propName)
{
    int idx = 0;
    QSettings settings;
    settings.beginGroup("gst");

    auto src = QGst::ElementFactory::make(elmName);
    if (!src) {
        QMessageBox::critical(this, windowTitle(), tr("Failed to create element '%1'").arg(elmName));
        return;
    }

    auto srcPad = src->getStaticPad("src");
    if (srcPad)
    {
        // To set the property, the device must be in Null state
        //
        src->setProperty(propName.toUtf8(), device);

        // To query the caps, the device must be in Ready state
        //
        src->setState(QGst::StateReady);
        src->getState(nullptr, nullptr, GST_SECOND * 10);

        caps = srcPad->caps();
        qDebug() << caps->toString();

        auto tuner = GST_TUNER(src);
        if (tuner)
        {
            auto selectedChannel = settings.value("video-channel").toString();

            // The list is owned by the GstTuner and must not be freed.
            //
            auto channelList = gst_tuner_list_channels(tuner);
            while (channelList)
            {
                auto ch = GST_TUNER_CHANNEL(channelList->data);
                //gst_tuner_set_channel(tuner, ch);

                listChannels->addItem(ch->label);
                if (selectedChannel == ch->label)
                {
                    idx = listChannels->count() - 1;
                }
                channelList = g_list_next(channelList);
            }
        }
        else
        {
            auto selectedChannel = settings.value("video-channel").toInt();
            if (elmName == "dv1394src")
            {
                for (int i = 1; i <= 64; ++i)
                {
                    listChannels->addItem(QString::number(i));
                }
                idx = selectedChannel - 1;
            }
            else if (elmName == "videotestsrc")
            {
                for (int i = 0; i <= 20; ++i)
                {
                    listChannels->addItem(QString::number(i));
                }
                idx = selectedChannel;
            }
        }

        // Now switch the device back to Null state (release resources)
        //
        src->setState(QGst::StateNull);
        src->getState(nullptr, nullptr, GST_SECOND * 10);
    }

    listChannels->setCurrentIndex(-1);
    listChannels->setCurrentIndex(idx);
}

static QString valueToString(const QGlib::Value& value)
{
    return GST_TYPE_FOURCC == value.type()? "(fourcc)" + value.toString():
           G_TYPE_STRING == value.type()?   "(string)" + value.toString():
           value.toString();
}

static QList<QGlib::Value> getFormats(const QGlib::Value& value)
{
    QList<QGlib::Value> formats;

    if (GST_TYPE_LIST == value.type() || GST_TYPE_ARRAY == value.type())
    {
        for (uint idx = 0; idx < gst_value_list_get_size(value); ++idx)
        {
            auto elm = gst_value_list_get_value(value, idx);
            formats << QGlib::Value(elm);
        }
    }
    else
    {
        formats << value;
    }

    return formats;
}

void VideoSourceDetails::inputChannelChanged(int index)
{
    listFormats->clear();
    listFormats->addItem(tr("(default)"));

    if (index < 0 || !caps)
    {
        return;
    }

    auto selectedFormat = QSettings().value("gst/format").toString();
    for (uint i = 0; i < caps->size(); ++i)
    {
        auto s = caps->internalStructure(i);
        Q_FOREACH (auto format, getFormats(s->value("format")))
        {
            auto formatName = !format.isValid()? s->name(): s->name().append(",format=").append(valueToString(format));
            if (listFormats->findData(formatName) >= 0)
            {
                continue;
            }
            auto displayName = !format.isValid()? s->name(): s->name().append(" (").append(format.toString()).append(")");
            listFormats->addItem(displayName, formatName);
            if (formatName == selectedFormat)
            {
                listFormats->setCurrentIndex(listFormats->count() - 1);
            }
        }
    }
}

static QGst::IntRange getRange(const QGlib::Value& value)
{
    // First, try extract a single int value (more common)
    //
    bool ok = false;
    int intValue = value.toInt(&ok);
    if (ok)
    {
        return QGst::IntRange(intValue, intValue);
    }

    // If failed, try extract a range value
    //
    return ok? QGst::IntRange(intValue, intValue): value.get<QGst::IntRange>();
}

void VideoSourceDetails::formatChanged(int index)
{
    listSizes->clear();
    listSizes->addItem(tr("(default)"));

    auto selectedFormat = listFormats->itemData(index).toString();
    if (index < 0 || !caps || selectedFormat.isEmpty())
    {
        return;
    }

    QList<QSize> sizes;
    auto selectedSize = QSettings().value("gst/size").toSize();
    for (uint i = 0; i < caps->size(); ++i)
    {
        auto s = caps->internalStructure(i);
        auto format = s->value("format");
        auto formatName = !format.isValid()? s->name(): s->name().append(",format=").append(valueToString(format));
        if (selectedFormat != formatName)
        {
            continue;
        }

        QGst::IntRange widthRange = getRange(s->value("width"));
        QGst::IntRange heightRange = getRange(s->value("height"));

        if (widthRange.end <= 0 || heightRange.end <= 0)
        {
            continue;
        }

        // Videotestsrc can produce frames till 2147483647x2147483647
        // Add some reasonable limits
        //
        widthRange .start = qMax(widthRange .start, 32);
        heightRange.start = qMax(heightRange.start, 24);
        widthRange .end = qMin(widthRange .end, 32768);
        heightRange.end = qMin(heightRange.end, 24576);

        // Iterate from min to max (160x120,320x240,640x480)
        //
        auto width = widthRange.start;
        auto height = heightRange.start;

        while (width <= widthRange.end && height <= heightRange.end)
        {
            sizes.append(QSize(width, height));
            width <<= 1;
            height <<= 1;
        }

        if ((width >> 1) != widthRange.end || (height >> 1) != heightRange.end)
        {
            // Iterate from max to min  (720x576,360x288,180x144)
            //
            width = widthRange.end;
            height = heightRange.end;
            while (width > widthRange.start && height > heightRange.start)
            {
                sizes.append(QSize(width, height));
                width >>= 1;
                height >>= 1;
            }
        }
    }

    // Sort resolutions descending
    //
    qSort(sizes.begin(), sizes.end(),
        [](const QSize &s1, const QSize &s2) { return s1.width() * s1.height() > s2.width() * s2.height(); }
        );

    // And remove duplicates
    //
    sizes.erase(std::unique (sizes.begin(), sizes.end()), sizes.end());

    Q_FOREACH(auto size, sizes)
    {
        QString name = tr("%1x%2").arg(size.width()).arg(size.height());
        listSizes->addItem(name, size);
        if (size == selectedSize)
        {
            listSizes->setCurrentIndex(listSizes->count() - 1);
        }
    }
}

// Return nullptr for 'default', otherwise the text itself
//
static QString getListText(const QComboBox* cb)
{
    auto idx = cb->currentIndex();
    return cb->itemData(idx).isNull()? nullptr: cb->itemText(idx);
}

static QVariant getListData(const QComboBox* cb)
{
    auto idx = cb->currentIndex();
    return cb->itemData(idx);
}
#if 0
void VideoSourceDetails::save(QSettings& settings)
{
    if (listVideoCodecs->count() == 0)
    {
        // The page was created, but never shown, so
        // all listboxes are empty.
        // Nothing has been changed, so exit right now
        //
        return;
    }

    settings.beginGroup("gst");

    auto device = getListData(listDevices).toStringList();
    if (device.isEmpty())
    {
        settings.remove("device-type");
        settings.remove("device");
    }
    else
    {
        settings.setValue("device-type", device.takeFirst());
        settings.setValue("device", device.takeFirst());
    }
    settings.setValue("video-channel", getListText(listChannels));
    settings.setValue("format",        getListData(listFormats));
    settings.setValue("size",          getListData(listSizes));
    settings.setValue("video-encoder", getListData(listVideoCodecs));
    settings.setValue("video-muxer",   getListData(listVideoMuxers));
    settings.setValue("rtp-payloader", getListData(listRtpPayloaders));
    settings.setValue("image-encoder", getListData(listImageCodecs));
    settings.setValue("enable-rtp",    checkEnableRtp->isChecked());
    settings.setValue("rtp-clients",   textRtpClients->text());
    settings.setValue("enable-http",   checkEnableHttp->isChecked());
    settings.setValue("http-push-url", textHttpPushUrl->text());
    settings.setValue("bitrate",       spinBitrate->value());
    settings.setValue("video-deinterlace", checkDeinterlace->isChecked());
    if (spinFps)
    {
        settings.setValue("limit-video-fps", checkFps->isChecked());
        settings.setValue("video-max-fps", spinFps->value() > 0? spinFps->value(): QVariant());
    }
    settings.endGroup();
}
#endif
