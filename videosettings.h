#ifndef VIDEOSETTINGS_H
#define VIDEOSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTextEdit;
class QCheckBox;
QT_END_NAMESPACE

class VideoSettings : public QWidget
{
    Q_OBJECT
    QComboBox* listDevices;
    QComboBox* listFormats;
    QComboBox* listSizes;
    QComboBox* listFramerates;
    QComboBox* listVideoCodecs;
    QComboBox* listVideoMuxers;
    QComboBox* listImageCodecs;
    QCheckBox* checkRecordAll;

    void updateDeviceList();
    void updateGstList(const char* setting, const char* def, unsigned long long type, QComboBox* cb);
    QString updatePipeline();

public:
    Q_INVOKABLE explicit VideoSettings(QWidget *parent = 0);
    
protected:
    virtual void showEvent(QShowEvent *);

signals:
    
public slots:
    void videoDeviceChanged(int index);
    void formatChanged(int index);
    void sizeChanged(int index);
    void save();

};

#endif // VIDEOSETTINGS_H
