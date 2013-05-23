#include "videosettings.h"
#include <QComboBox>
#include <QCheckBox>
#include <QDebug>
#include <QFormLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTextEdit>

// Video encoder
//
#include <QGlib/Error>
#include <QGst/ElementFactory>
#include <QGst/Caps>
#include <QGst/Pad>
#include <QGst/Parse>
#include <QGst/Pipeline>
#include <QGst/PropertyProbe>
#include <QGst/Structure>
#include <gst/gst.h>

#ifdef QT_ARCH_WINDOWS
#define PLATFORM_SPECIFIC_SOURCE "dshowvideosrc"
#else
#define PLATFORM_SPECIFIC_SOURCE "v4l2src"
#endif

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
    layout->addRow(tr("Video &codec"), listVideoCodecs = new QComboBox());
    layout->addRow(tr("Video &bitrate"), spinBitrate = new QSpinBox());
    spinBitrate->setRange(300, 102400);
    spinBitrate->setSingleStep(100);
    spinBitrate->setSuffix(tr(" kbit per second"));
    spinBitrate->setValue(settings.value("bitrate", 4000).toInt());

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
    updateGstList("video-encoder", "x264enc", GST_ELEMENT_FACTORY_TYPE_ENCODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO, listVideoCodecs);
    updateGstList("video-mux", "mpegtsmux", GST_ELEMENT_FACTORY_TYPE_MUXER, listVideoMuxers);
    updateGstList("image-encoder", "jpegenc", GST_ELEMENT_FACTORY_TYPE_ENCODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE, listImageCodecs);
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
    auto selectedDevice = QSettings().value("device").toString();

    QGst::ElementPtr src = QGst::ElementFactory::make(PLATFORM_SPECIFIC_SOURCE);
    if (!src) {
        QMessageBox::critical(this, windowTitle(), tr("Failed to create element \"%1\"").arg(PLATFORM_SPECIFIC_SOURCE));
        return;
    }

    src->setState(QGst::StateReady);
    QGst::PropertyProbePtr propertyProbe = src.dynamicCast<QGst::PropertyProbe>();

    // Look for device-name instead of "device" since "device" may be different, thanks to PNP.
    //
    if (propertyProbe && propertyProbe->propertySupportsProbe("device-name"))
    {
        //get a list of devices that the element supports
        QList<QGlib::Value> devices = propertyProbe->probeAndGetValues("device-name");
        Q_FOREACH(const QGlib::Value& device, devices)
        {
            QString deviceName = device.toString();
            QGst::PadPtr srcPad = src->getStaticPad("src");
            if (srcPad)
            {
                //add the device on the combobox
                listDevices->addItem(device.toString(), srcPad->caps()->toString());
            }
        }
    }
}

void VideoSettings::videoDeviceChanged(int index)
{
    listFormats->clear();
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
        QString formatName = s->name().append(",format=(fourcc)").append(format);
        if (listFormats->findData(formatName) >= 0)
        {
            continue;
        }
        QString displayName = s->name().append(" (").append(format).append(")");
        listFormats->addItem(displayName, formatName);
        if (format == selectedFormat)
        {
            listFormats->setCurrentIndex(listFormats->count() - 1);
        }
    }
    updatePipeline();
}

void VideoSettings::formatChanged(int index)
{
    listSizes->clear();

    QGst::CapsPtr caps = QGst::Caps::fromString(listDevices->itemData(listDevices->currentIndex()).toString());
    if (index < 0 || !caps)
    {
        return;
    }

    auto selectedFormat = listFormats->itemData(index).toString();
    auto selectedSize = QSettings().value("size").toSize();
    for (uint i = 0; i < caps->size(); ++i)
    {
        QGst::StructurePtr s = caps->internalStructure(i);
        QString formatName = s->name().append(",format=(fourcc)").append(s->value("format").toString());
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
    updatePipeline();
}

QString VideoSettings::updatePipeline()
{
    auto devicePath = listDevices->itemText(listDevices->currentIndex());
    auto format = listFormats->itemData(listFormats->currentIndex()).toString();
    auto size = listSizes->itemData(listSizes->currentIndex()).toSize();
    QString str(PLATFORM_SPECIFIC_SOURCE);

    if (!devicePath.isNull())
    {
        str.append(" device-name=\"").append(devicePath).append("\"");
        if (!format.isNull())
        {
            str.append(" ! ").append(format);
            if (size.width() > 0 && size.height() > 0)
            {
                str = str.append(",width=(int)%1,height=(int)%2").arg(size.width()).arg(size.height());
            }
        }
    }

    return str;
}

void VideoSettings::save()
{
    QSettings settings;
    settings.setValue("device", listDevices->itemText(listDevices->currentIndex()));
    settings.setValue("format", listFormats->itemData(listFormats->currentIndex()));
    settings.setValue("size", listSizes->itemData(listSizes->currentIndex()));
    settings.setValue("video-encoder", listVideoCodecs->itemData(listVideoCodecs->currentIndex()));
    settings.setValue("video-mux",   listVideoMuxers->itemData(listVideoMuxers->currentIndex()));
    settings.setValue("image-encoder", listImageCodecs->itemData(listImageCodecs->currentIndex()));
    settings.setValue("enable-video", checkRecordAll->isChecked());

    settings.setValue("src", updatePipeline());
    settings.setValue("bitrate", spinBitrate->value());
}
