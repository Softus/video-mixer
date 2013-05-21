#include "videosettings.h"
#include <QComboBox>
#include <QCheckBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QPushButton>
#include <QSettings>
#include <QTextEdit>

// V4l2src
//
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>

// Video encoder
//
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
    connect(listSizes, SIGNAL(currentIndexChanged(int)), this, SLOT(sizeChanged(int)));
    layout->addRow(tr("Frame &rate"), listFramerates = new QComboBox());
    layout->addRow(tr("Video &codec"), listVideoCodecs = new QComboBox());
    layout->addRow(tr("Video &muxer"), listVideoMuxers = new QComboBox());
    layout->addRow(tr("&Image codec"), listImageCodecs = new QComboBox());
    layout->addRow(nullptr, checkRecordAll = new QCheckBox(tr("Record entire &video")));
    checkRecordAll->setChecked(settings.value("enable-video").toBool());
    setLayout(layout);
}

void VideoSettings::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    // Refill the boxes every time the page is shown
    //
    updateGstList("videocodec", "x264enc", GST_ELEMENT_FACTORY_TYPE_ENCODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO, listVideoCodecs);
    updateGstList("videomux", "mpegtsmux", GST_ELEMENT_FACTORY_TYPE_MUXER, listVideoMuxers);
    updateGstList("imagecodec", "jpegenc", GST_ELEMENT_FACTORY_TYPE_ENCODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE, listImageCodecs);
    updateDeviceList();
}

void VideoSettings::updateGstList(const char* setting, const char* def, unsigned long long type, QComboBox* cb)
{
    cb->clear();
    auto selectedCodec = QSettings().value(setting, def).toString();

    GList* encoders = gst_element_factory_list_get_elements(
        type, GST_RANK_NONE);

    for (GList* curr = encoders; curr; curr = curr->next)
    {
        GstElementFactory *factory = (GstElementFactory *)curr->data;
        QString longName(factory->details.longname);
        QString codecName(factory->parent.object.name);
        cb->addItem(longName, codecName);
        if (selectedCodec == codecName)
        {
            cb->setCurrentIndex(cb->count() - 1);
        }
    }

    gst_plugin_feature_list_free(encoders);
}

QString VideoSettings::updatePipeline()
{
    auto devicePath = listDevices->itemData(listDevices->currentIndex()).toString();
    auto format = listFormats->itemData(listFormats->currentIndex()).toUInt();
    auto strFormat = QString::fromAscii((const char*)&format, sizeof(int));

    QString mimetype;
    if (V4L2_PIX_FMT_GREY == format)
    {
        mimetype = "video/x-raw-gray";
    }
    else
    {
        switch (format & 0xFF)
        {
        case 'A':
        case 'R':
        case 'G':
        case 'B':
            mimetype = "video/x-raw-rgb";
            break;
        case 'P':
            mimetype = "video/x-raw-palette"; // Oh yes!
            break;
        case 'd':
            mimetype = "video/x-dv";
        case 'J':
            mimetype = "image/jpeg";
            break;
        case 'M':
        case 'H':
        case 'X':
            mimetype = "video/mpegts";
            break;
        default:
            mimetype = "video/x-raw-yuv"; // The common case
            break;
        }
    }

    auto size = listSizes->itemData(listSizes->currentIndex()).toSize();
    auto rate = listFramerates->itemData(listFramerates->currentIndex()).toSize();
    QString str;

    if (!devicePath.isNull())
    {
        str.append("v4l2src device=").append(devicePath);
        if (format)
        {
            str.append(" ! ").append(mimetype).append(",format=").append(strFormat);
            if (size.width() > 0 && size.height() > 0)
            {
                str = str.append(",width=%1,height=%2").arg(size.width()).arg(size.height());
            }
            if (rate.width() > 0 && rate.height() > 0)
            {
                str = str.append(",framerate=%1/%2").arg(rate.width()).arg(rate.height());
            }
        }
    }

    return str;
}

void VideoSettings::updateDeviceList()
{
    listDevices->clear();
    auto selectedDevice = QSettings().value("device").toString();
    QDir dir("/dev","video*", QDir::Name, QDir::System);
    foreach (auto entry, dir.entryInfoList())
    {
        QFile dev(entry.absoluteFilePath());
        if (dev.open(QFile::ReadWrite))
        {
            v4l2_capability vcap;
            if (ioctl(dev.handle(), VIDIOC_QUERYCAP, &vcap) >= 0 && (V4L2_CAP_VIDEO_CAPTURE & vcap.capabilities))
            {
                auto devicePath = entry.absoluteFilePath();
                auto deviceName = QString::fromLocal8Bit((const char*)vcap.card);
                listDevices->addItem(deviceName.append(" (").append(devicePath).append(")"), devicePath);
                if (devicePath == selectedDevice)
                {
                    listDevices->setCurrentIndex(listDevices->count() - 1);
                }
            }
        }
    }
}

void VideoSettings::videoDeviceChanged(int index)
{
    listFormats->clear();

    if (index < 0)
    {
        return;
    }

    auto selectedFormat = QSettings().value("format").toUInt();
    QFile dev(listDevices->itemData(index).toString());
    if (dev.open(QFile::ReadWrite))
    {
        listFormats->addItem(tr("(default)"), 0);

        v4l2_fmtdesc fmt;
        fmt.index = 0;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while (ioctl(dev.handle(), VIDIOC_ENUM_FMT, &fmt) >= 0)
        {
            auto description = QString::fromLocal8Bit((const char*)fmt.description);
            listFormats->addItem(description, fmt.pixelformat);
            if (fmt.pixelformat == selectedFormat)
            {
                listFormats->setCurrentIndex(listFormats->count() - 1);
            }
            ++fmt.index;
        }
    }
    updatePipeline();
}

void VideoSettings::formatChanged(int index)
{
    listSizes->clear();

    if (index < 0)
    {
        return;
    }

    auto selectedSize = QSettings().value("size").toSize();
    QFile dev(listDevices->itemData(listDevices->currentIndex()).toString());
    if (dev.open(QFile::ReadWrite))
    {
        listSizes->addItem(tr("(default)"), QSize(0, 0));

        v4l2_frmsizeenum frmsize;
        frmsize.pixel_format = listFormats->itemData(listFormats->currentIndex()).toUInt();
        frmsize.index = 0;

        while (ioctl(dev.handle(), VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0)
        {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
            {
                auto dimensions = QString(tr("%1x%2")).arg(frmsize.discrete.width).arg(frmsize.discrete.height);
                auto size = QSize(frmsize.discrete.width, frmsize.discrete.height);
                listSizes->addItem(dimensions, size);
                if (size == selectedSize)
                {
                    listSizes->setCurrentIndex(listSizes->count() - 1);
                }
            }

            ++frmsize.index;
        }
    }
    updatePipeline();
}

void VideoSettings::sizeChanged(int index)
{
    listFramerates->clear();

    if (index < 0)
    {
        return;
    }

    // 25 by default, if not present, the first will be selected (usually, 30)
    //
    auto selectedRate = QSettings().value("rate", QSize(25, 1)).toSize();
    QFile dev(listDevices->itemData(listDevices->currentIndex()).toString());
    if (dev.open(QFile::ReadWrite))
    {
        listFramerates->addItem(tr("(default)"), QSize(0, 0));

        v4l2_frmivalenum frmival;
        auto size = listSizes->itemData(listSizes->currentIndex()).toSize();

        frmival.pixel_format = listFormats->itemData(listFormats->currentIndex()).toUInt();
        frmival.width = size.width();
        frmival.height = size.height();
        frmival.index = 0;

        while (ioctl(dev.handle(), VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0)
        {
            if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
            {
                auto rate = QString(tr("%1 fps")).arg((1.0 * frmival.discrete.denominator) / frmival.discrete.numerator);
                auto ratex = QSize(frmival.discrete.denominator, frmival.discrete.numerator);
                listFramerates->addItem(rate, ratex);
                if (ratex == selectedRate)
                {
                    listFramerates->setCurrentIndex(listFramerates->count() - 1);
                }
            }

            ++frmival.index;
        }
    }
    updatePipeline();
}

void VideoSettings::save()
{
    QSettings settings;
    settings.setValue("device", listDevices->itemData(listDevices->currentIndex()));
    settings.setValue("size", listFormats->itemData(listFormats->currentIndex()));
    settings.setValue("size", listSizes->itemData(listSizes->currentIndex()));
    settings.setValue("rate", listFramerates->itemData(listFramerates->currentIndex()));
    settings.setValue("src", updatePipeline());
    settings.setValue("videocodec", listVideoCodecs->itemData(listVideoCodecs->currentIndex()));
    settings.setValue("videomux",   listVideoMuxers->itemData(listVideoMuxers->currentIndex()));
    settings.setValue("imagecodec", listImageCodecs->itemData(listImageCodecs->currentIndex()));
    settings.setValue("enable-video", checkRecordAll->isChecked());
}
