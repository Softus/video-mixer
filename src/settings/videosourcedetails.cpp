/*
 * Copyright (C) 2013-2015 Irkutsk Diagnostic Center.
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
#include <QxtLineEdit>

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

static QString getPropName(const QString& deviceType)
{
    if (deviceType == "videotestsrc")  return "pattern";
    if (deviceType == "dshowvideosrc") return "device-name";
    if (deviceType == "v4l2src")       return "device";
    if (deviceType == "osxvideosrc")   return "device";

    // Unknown device type
    return QString();
}

// TODO: rewrite with qvariantmap
VideoSourceDetails::VideoSourceDetails(const QVariantMap& parameters, QWidget *parent)
  : QDialog(parent)
  , checkFps(nullptr)
  , spinFps(nullptr)
{
    selectedChannel = parameters.value("video-channel");
    selectedFormat  = parameters.value("format").toString();
    selectedSize    = parameters.value("size").toSize();

    auto layoutMain = new QFormLayout();

    editAlias= new QLineEdit(parameters.value("alias").toString());
    layoutMain->addRow(tr("&Alias"), editAlias);

#ifdef WITH_DICOM
    // Modality override
    //
    editModality  = new QxtLineEdit(parameters.value("modality").toString());
    editModality->setSampleText(tr("(default)"));
    layoutMain->addRow(tr("&Modality"), editModality);
#endif

    layoutMain->addRow(tr("I&nput channel"), listChannels = new QComboBox());
    listChannels->addItem(tr("(default)"));
    connect(listChannels, SIGNAL(currentIndexChanged(int)), this, SLOT(inputChannelChanged(int)));
    layoutMain->addRow(tr("Pixel &format"), listFormats = new QComboBox());
    connect(listFormats, SIGNAL(currentIndexChanged(int)), this, SLOT(formatChanged(int)));
    layoutMain->addRow(tr("Frame &size"), listSizes = new QComboBox());
    layoutMain->addRow(tr("Video &codec"), listVideoCodecs = new QComboBox());

    auto elm = QGst::ElementFactory::make("videorate");
    if (elm && elm->findProperty("max-rate"))
    {
        layoutMain->addRow(checkFps = new QCheckBox(tr("&Limit rate")), spinFps = new QSpinBox());
        connect(checkFps, SIGNAL(toggled(bool)), spinFps, SLOT(setEnabled(bool)));
        checkFps->setChecked(parameters.value("limit-video-fps", DEFAULT_LIMIT_VIDEO_FPS).toBool());

        spinFps->setRange(1, 200);
        spinFps->setValue(parameters.value("video-max-fps", DEFAULT_VIDEO_MAX_FPS).toInt());
        spinFps->setSuffix(tr(" frames per second"));
        spinFps->setEnabled(checkFps->isChecked());
    }
    layoutMain->addRow(tr("Video &bitrate"), spinBitrate = new QSpinBox());
    spinBitrate->setRange(0, 102400);
    spinBitrate->setSingleStep(100);
    spinBitrate->setSuffix(tr(" kbit per second"));
    spinBitrate->setValue(parameters.value("bitrate", DEFAULT_VIDEOBITRATE).toInt());

    layoutMain->addRow(nullptr, checkDeinterlace = new QCheckBox(tr("De&interlace")));
    checkDeinterlace->setChecked(parameters.value("video-deinterlace").toBool());

    layoutMain->addRow(tr("Video m&uxer"), listVideoMuxers = new QComboBox());
    layoutMain->addRow(tr("Ima&ge codec"), listImageCodecs = new QComboBox());
    layoutMain->addRow(tr("RTP &payloader"), listRtpPayloaders = new QComboBox());

    // UDP streaming
    //
    editRtpClients = new QLineEdit(parameters.value("rtp-clients").toString());
    layoutMain->addRow(checkEnableRtp = new QCheckBox(tr("&RTP clients")), editRtpClients);
    connect(checkEnableRtp, SIGNAL(toggled(bool)), editRtpClients, SLOT(setEnabled(bool)));
    checkEnableRtp->setChecked(parameters.value("enable-rtp").toBool());
    editRtpClients->setEnabled(checkEnableRtp->isChecked());

    // Http streaming
    //
    editHttpPushUrl = new QLineEdit(parameters.value("http-push-url").toString());
    layoutMain->addRow(checkEnableHttp = new QCheckBox(tr("&Http push URL")), editHttpPushUrl);
    connect(checkEnableHttp, SIGNAL(toggled(bool)), editHttpPushUrl, SLOT(setEnabled(bool)));
    checkEnableHttp->setChecked(parameters.value("enable-http").toBool());
    editHttpPushUrl->setEnabled(checkEnableHttp->isChecked());

    // Buttons row
    //
    auto layoutBtns = new QHBoxLayout;
    layoutBtns->addStretch(1);
    auto btnCancel = new QPushButton(tr("Cancel"));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
    layoutBtns->addWidget(btnCancel);
    auto btnSave = new QPushButton(tr("Save"));
    connect(btnSave, SIGNAL(clicked()), this, SLOT(accept()));
    btnSave->setDefault(true);
    layoutBtns->addWidget(btnSave);
    layoutMain->addRow(layoutBtns);

    setLayout(layoutMain);

    // Refill the boxes every time the page is shown
    //
    auto selectedCodec = updateGstList(parameters, "video-encoder", DEFAULT_VIDEO_ENCODER, GST_ELEMENT_FACTORY_TYPE_ENCODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO, listVideoCodecs);
    listVideoCodecs->insertItem(0, tr("(none)"));
    if (selectedCodec.isEmpty())
    {
        listVideoCodecs->setCurrentIndex(0);
    }
    auto selectedMuxer = updateGstList(parameters, "video-muxer",   DEFAULT_VIDEO_MUXER,   GST_ELEMENT_FACTORY_TYPE_MUXER, listVideoMuxers);
    listVideoMuxers->insertItem(0, tr("(none)"));
    if (selectedMuxer.isEmpty())
    {
        listVideoMuxers->setCurrentIndex(0);
    }
    updateGstList(parameters, "image-encoder", DEFAULT_IMAGE_ENCODER, GST_ELEMENT_FACTORY_TYPE_ENCODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE, listImageCodecs);
    updateGstList(parameters, "rtp-payloader", DEFAULT_RTP_PAYLOADER, GST_ELEMENT_FACTORY_TYPE_PAYLOADER, listRtpPayloaders);
}

QString VideoSourceDetails::updateGstList(const QVariantMap& parameters, const char* settingName, const char* def, unsigned long long type, QComboBox* cb)
{
    cb->clear();
    auto selectedCodec = parameters.value(settingName, def).toString();
    auto extra = parameters.value(QString(settingName)+"-extra").toBool() || qApp->keyboardModifiers().testFlag(Qt::ShiftModifier);
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

void VideoSourceDetails::updateDevice(const QString& device, const QString& deviceType)
{
    int idx = 0;

    auto src = QGst::ElementFactory::make(deviceType);
    if (!src)
    {
        QMessageBox::critical(this, windowTitle(), tr("Failed to create element '%1'").arg(deviceType));
        return;
    }

    auto propName = getPropName(deviceType);
    auto srcPad = src->getStaticPad("src");
    if (srcPad)
    {
        if (!propName.isEmpty())
        {
            // To set this property, the device must be in Null state
            //
            src->setProperty(propName.toUtf8(), device);
        }

        // To query the caps, the device must be in Ready state
        //
        src->setState(QGst::StateReady);
        src->getState(nullptr, nullptr, GST_SECOND * 10);

        caps = srcPad->caps();
        qDebug() << caps->toString();

        auto tuner = GST_TUNER(src);
        if (tuner)
        {
            auto selectedChannelLabel = selectedChannel.toString();
            // The list is owned by the GstTuner and must not be freed.
            //
            auto channelList = gst_tuner_list_channels(tuner);
            while (channelList)
            {
                auto ch = GST_TUNER_CHANNEL(channelList->data);
                //gst_tuner_set_channel(tuner, ch);

                listChannels->addItem(ch->label);
                if (selectedChannelLabel == ch->label)
                {
                    idx = listChannels->count() - 1;
                }
                channelList = g_list_next(channelList);
            }
        }
        else
        {
            if (deviceType == "dv1394src")
            {
                for (int i = 1; i <= 64; ++i)
                {
                    listChannels->addItem(QString::number(i));
                }
                idx = selectedChannel.toInt();
            }
            else if (deviceType == "videotestsrc")
            {
#if GST_CHECK_VERSION(1,0,0)
                auto numPatterns = 22;
#else
                auto numPatterns = 20;
#endif
                for (int i = 0; i <= numPatterns; ++i)
                {
                    listChannels->addItem(QString::number(i));
                }
                idx = selectedChannel.toInt() + 1;
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

    for (uint i = 0; i < caps->size(); ++i)
    {
        auto s = caps->internalStructure(i);
        foreach (auto format, getFormats(s->value("format")))
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

    foreach (auto size, sizes)
    {
        QString name = tr("%1x%2").arg(size.width()).arg(size.height());
        listSizes->addItem(name, size);
        if (size == selectedSize)
        {
            listSizes->setCurrentIndex(listSizes->count() - 1);
        }
    }
}

// Return nullptr for '(default)', otherwise the text itself
//
static QString getListText(const QComboBox* cb)
{
    auto idx = cb->currentIndex();
    return idx <= 0? nullptr: cb->itemText(idx);
}

static QVariant getListData(const QComboBox* cb)
{
    auto idx = cb->currentIndex();
    return cb->itemData(idx);
}

void VideoSourceDetails::updateParameters(QVariantMap& settings)
{
    settings["alias"]             = editAlias->text();
#ifdef WITH_DICOM
    settings["modality"]          = editModality->text();
#endif
    settings["video-channel"]     = getListText(listChannels);
    settings["format"]            = getListData(listFormats);
    settings["size"]              = getListData(listSizes);
    settings["video-encoder"]     = getListData(listVideoCodecs);
    settings["video-muxer"]       = getListData(listVideoMuxers);
    settings["rtp-payloader"]     = getListData(listRtpPayloaders);
    settings["image-encoder"]     = getListData(listImageCodecs);
    settings["enable-rtp"]        = checkEnableRtp->isChecked();
    settings["rtp-clients"]       = editRtpClients->text();
    settings["enable-http"]       = checkEnableHttp->isChecked();
    settings["http-push-url"]     = editHttpPushUrl->text();
    settings["bitrate"]           = spinBitrate->value();
    settings["video-deinterlace"] = checkDeinterlace->isChecked();

    if (spinFps)
    {
        settings["limit-video-fps"] = checkFps->isChecked();
        settings["video-max-fps"] = spinFps->value() > 0? spinFps->value(): QVariant();
    }
}
