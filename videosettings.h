#ifndef VIDEOSETTINGS_H
#define VIDEOSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTextEdit;
class QCheckBox;
class QSpinBox;
QT_END_NAMESPACE

#ifdef QT_ARCH_WINDOWS
#define PLATFORM_SPECIFIC_SOURCE "dshowvideosrc"
#define PLATFORM_SPECIFIC_PROPERTY "device-name"
#else
#define PLATFORM_SPECIFIC_SOURCE "v4l2src"
#define PLATFORM_SPECIFIC_PROPERTY "device"
#endif

#define DEFAULT_VIDEOBITRATE 4000

class VideoSettings : public QWidget
{
    Q_OBJECT
    QComboBox* listDevices;
    QComboBox* listFormats;
    QComboBox* listSizes;
    QComboBox* listVideoCodecs;
    QComboBox* listVideoMuxers;
    QComboBox* listImageCodecs;
    QCheckBox* checkRecordAll;
    QSpinBox* spinBitrate;

    void updateDeviceList();
    void updateGstList(const char* setting, const char* def, unsigned long long type, QComboBox* cb);

public:
    Q_INVOKABLE explicit VideoSettings(QWidget *parent = 0);

protected:
    virtual void showEvent(QShowEvent *);

signals:
    
public slots:
    void videoDeviceChanged(int index);
    void formatChanged(int index);
    void save();
};

#endif // VIDEOSETTINGS_H
