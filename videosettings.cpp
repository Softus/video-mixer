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
    spinBitrate->setValue(settings.value("bitrate", DEFAULT_VIDEOBITRATE).toInt());

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
    listDevices->addItem(tr("(default)"));

    auto selectedDevice = QSettings().value("device").toString();

    QGst::ElementPtr src = QGst::ElementFactory::make(PLATFORM_SPECIFIC_SOURCE);
    if (!src) {
        QMessageBox::critical(this, windowTitle(), tr("Failed to create element \"%1\"").arg(PLATFORM_SPECIFIC_SOURCE));
        return;
    }

    src->setState(QGst::StateReady);
    src->getState(NULL, NULL, GST_SECOND * 10);

    // Look for device-name for windows and "device" for linux/macosx
    //
    QGst::PropertyProbePtr propertyProbe = src.dynamicCast<QGst::PropertyProbe>();
    if (propertyProbe && propertyProbe->propertySupportsProbe(PLATFORM_SPECIFIC_PROPERTY))
    {
        //get a list of devices that the element supports
        QList<QGlib::Value> devices = propertyProbe->probeAndGetValues(PLATFORM_SPECIFIC_PROPERTY);
        Q_FOREACH(const QGlib::Value& device, devices)
        {
            QString deviceName = device.toString();
            QGst::PadPtr srcPad = src->getStaticPad("src");
            if (srcPad)
            {
                //add the device on the combobox
                listDevices->addItem(deviceName, srcPad->caps()->toString());
                if (selectedDevice == deviceName)
                {
                    listDevices->setCurrentIndex(listDevices->count() - 1);
                }
            }
        }
    }
    src->setState(QGst::StateNull);
    src->getState(NULL, NULL, GST_SECOND * 10);
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
    settings.setValue("video-mux",   listVideoMuxers->itemData(listVideoMuxers->currentIndex()));
    settings.setValue("image-encoder", listImageCodecs->itemData(listImageCodecs->currentIndex()));
    settings.setValue("enable-video", checkRecordAll->isChecked());

    settings.setValue("bitrate", spinBitrate->value());
}
