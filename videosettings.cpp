/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
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

#include "videosettings.h"
#include "defaults.h"
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
#include <QGst/Caps>
#include <QGst/ElementFactory>
#include <QGst/Pad>
#include <QGst/Parse>
#include <QGst/Pipeline>
#include <QGst/PropertyProbe>
#include <QGst/Structure>
#include <gst/gst.h>

VideoSettings::VideoSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    auto layout = new QFormLayout();

    layout->addRow(tr("Video &device"), listDevices = new QComboBox());
    connect(listDevices, SIGNAL(currentIndexChanged(int)), this, SLOT(videoDeviceChanged(int)));
    layout->addRow(tr("Pixel &format"), listFormats = new QComboBox());
    connect(listFormats, SIGNAL(currentIndexChanged(int)), this, SLOT(formatChanged(int)));
    layout->addRow(tr("Frame &size"), listSizes = new QComboBox());
    layout->addRow(tr("Video &codec"), listVideoCodecs = new QComboBox());
    layout->addRow(tr("Video &bitrate"), spinBitrate = new QSpinBox());
    spinBitrate->setRange(300, 102400);
    spinBitrate->setSingleStep(100);
    spinBitrate->setSuffix(tr(" kbit per second"));
    spinBitrate->setValue(settings.value("bitrate", DEFAULT_VIDEOBITRATE).toInt());
    checkDeinterlace = new QCheckBox(tr("De&interlace"));
    checkDeinterlace->setChecked(settings.value("video-deinterlace").toBool());
    layout->addRow(nullptr, checkDeinterlace);

    layout->addRow(tr("Video &muxer"), listVideoMuxers = new QComboBox());
    layout->addRow(tr("&Image codec"), listImageCodecs = new QComboBox());
    layout->addRow(nullptr, checkRecordAll = new QCheckBox(tr("Record entire &video")));
    checkRecordAll->setChecked(settings.value("enable-video").toBool());
    layout->addRow(tr("RTP &payloader"), listRtpPayloaders = new QComboBox());
    textRtpClients = new QLineEdit(settings.value("rtp-clients").toString());
    layout->addRow(tr("&RTP clients"), textRtpClients);
    checkEnableRtp = new QCheckBox(tr("&Enable RTP"));
    checkEnableRtp->setChecked(settings.value("enable-rtp").toBool());
    layout->addRow(nullptr, checkEnableRtp);
    setLayout(layout);
}

void VideoSettings::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    // Refill the boxes every time the page is shown
    //
    updateGstList("video-encoder", DEFAULT_VIDEO_ENCODER, GST_ELEMENT_FACTORY_TYPE_ENCODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO, listVideoCodecs);
    updateGstList("video-muxer",   DEFAULT_VIDEO_MUXER,   GST_ELEMENT_FACTORY_TYPE_MUXER, listVideoMuxers);
    updateGstList("image-encoder", DEFAULT_IMAGE_ENCODER, GST_ELEMENT_FACTORY_TYPE_ENCODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE, listImageCodecs);
    updateGstList("rtp-payloader", DEFAULT_RTP_PAYLOADER, GST_ELEMENT_FACTORY_TYPE_PAYLOADER, listRtpPayloaders);

    updateDeviceList();
}

void VideoSettings::updateGstList(const char* setting, const char* def, unsigned long long type, QComboBox* cb)
{
    cb->clear();
    auto selectedCodec = QSettings().value(setting, def).toString();

    GList* encoders = gst_element_factory_list_get_elements(type, GST_RANK_NONE);
    for (GList* curr = encoders; curr; curr = curr->next)
    {
        GstElementFactory *factory = (GstElementFactory *)curr->data;
        QString longName(factory->details.longname);
        QString codecName(factory->parent.name);
        cb->addItem(longName, codecName);
        if (selectedCodec == codecName)
        {
            cb->setCurrentIndex(cb->count() - 1);
        }
    }

    gst_plugin_feature_list_free(encoders);
}

void VideoSettings::updateDeviceList()
{
    listDevices->clear();
    listDevices->addItem(tr("(default)"));

    auto selectedDevice = QSettings().value("device").toString();
    auto src = QGst::ElementFactory::make(PLATFORM_SPECIFIC_SOURCE);
    if (!src) {
        QMessageBox::critical(this, windowTitle(), tr("Failed to create element '%1'").arg(PLATFORM_SPECIFIC_SOURCE));
        return;
    }

    // Look for device-name for windows and "device" for linux/macosx
    //
    QGst::PropertyProbePtr propertyProbe = src.dynamicCast<QGst::PropertyProbe>();
    if (propertyProbe && propertyProbe->propertySupportsProbe(PLATFORM_SPECIFIC_PROPERTY))
    {
        //get a list of devices that the element supports
        auto devices = propertyProbe->probeAndGetValues(PLATFORM_SPECIFIC_PROPERTY);
        foreach (const QGlib::Value& device, devices)
        {
            auto deviceName = device.toString();
            auto srcPad = src->getStaticPad("src");
            if (srcPad)
            {
                // To set the property, the device must be in Null state
                //
                src->setProperty(PLATFORM_SPECIFIC_PROPERTY, device);

                // To query the caps, the device must be in Ready state
                //
                src->setState(QGst::StateReady);
                src->getState(nullptr, nullptr, GST_SECOND * 10);

                //qDebug() << deviceName << " caps:\n" << srcPad->caps()->toString();

                // Add the device and its caps to the combobox
                //
                listDevices->addItem(deviceName, srcPad->caps()->toString());
                if (selectedDevice == deviceName)
                {
                    listDevices->setCurrentIndex(listDevices->count() - 1);
                }

                // Now switch back to Null state for next device
                //
                src->setState(QGst::StateNull);
                src->getState(nullptr, nullptr, GST_SECOND * 10);
            }
        }
    }
}

void VideoSettings::videoDeviceChanged(int index)
{
    listFormats->clear();
    listFormats->addItem(tr("(default)"));

    QGst::CapsPtr caps = QGst::Caps::fromString(listDevices->itemData(index).toString());

    if (index < 0 || !caps)
    {
        return;
    }

    auto selectedFormat = QSettings().value("format").toString();
    for (uint i = 0; i < caps->size(); ++i)
    {
        QGst::StructurePtr s = caps->internalStructure(i);
        QString format = s->value("format").toString();
        QString formatName = format.isEmpty()? s->name(): s->name().append(",format=(fourcc)").append(format);
        if (listFormats->findData(formatName) >= 0)
        {
            continue;
        }
        QString displayName = format.isEmpty()? s->name(): s->name().append(" (").append(format).append(")");
        listFormats->addItem(displayName, formatName);
        if (formatName == selectedFormat)
        {
            listFormats->setCurrentIndex(listFormats->count() - 1);
        }
    }
}

void VideoSettings::formatChanged(int index)
{
    listSizes->clear();
    listSizes->addItem(tr("(default)"));

    QGst::CapsPtr caps = QGst::Caps::fromString(listDevices->itemData(listDevices->currentIndex()).toString());
    QString selectedFormat = listFormats->itemData(index).toString();
    if (index < 0 || !caps || selectedFormat.isEmpty())
    {
        return;
    }

    auto selectedSize = QSettings().value("size").toSize();
    for (uint i = 0; i < caps->size(); ++i)
    {
        QGst::StructurePtr s = caps->internalStructure(i);
        QString format = s->value("format").toString();
        QString formatName = format.isEmpty()? s->name(): s->name().append(",format=(fourcc)").append(format);
        if (selectedFormat != formatName)
        {
            continue;
        }

        QSize size(s->value("width").toInt(), s->value("height").toInt());
        if (listSizes->findData(size) >= 0)
        {
            continue;
        }
        QString name = tr("%1x%2").arg(size.width()).arg(size.height());
        listSizes->addItem(name, size);
        if (size == selectedSize)
        {
            listSizes->setCurrentIndex(listSizes->count() - 1);
        }
    }
}

void VideoSettings::save()
{
    QSettings settings;
    settings.setValue("device", listDevices->itemData(listDevices->currentIndex()).isNull()?
                        nullptr: listDevices->itemText(listDevices->currentIndex()));
    settings.setValue("format", listFormats->itemData(listFormats->currentIndex()));
    settings.setValue("size", listSizes->itemData(listSizes->currentIndex()));
    settings.setValue("video-encoder", listVideoCodecs->itemData(listVideoCodecs->currentIndex()));
    settings.setValue("video-muxer",   listVideoMuxers->itemData(listVideoMuxers->currentIndex()));
    settings.setValue("rtp-payloader",   listRtpPayloaders->itemData(listRtpPayloaders->currentIndex()));
    settings.setValue("image-encoder", listImageCodecs->itemData(listImageCodecs->currentIndex()));
    settings.setValue("enable-video", checkRecordAll->isChecked());
    settings.setValue("enable-rtp", checkEnableRtp->isChecked());
    settings.setValue("rtp-clients", textRtpClients->text());
    settings.setValue("video-deinterlace", checkDeinterlace->isChecked());

    settings.setValue("bitrate", spinBitrate->value());
}
