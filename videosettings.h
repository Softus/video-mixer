#ifndef VIDEOSETTINGS_H
#define VIDEOSETTINGS_H

#include <QWidget>

class QComboBox;
class QTextEdit;

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
    QTextEdit* pipeline;

    void updateDeviceList();
    void updateGstList(const char* setting, const char* def, unsigned long long type, QComboBox* cb);
    void updatePipeline();

public:
    explicit VideoSettings(QWidget *parent = 0);
    
protected:
    virtual void showEvent(QShowEvent *);

signals:
    
public slots:
    void videoDeviceChanged(int index);
    void formatChanged(int index);
    void sizeChanged(int index);
    void rateChanged(int index);
    void save();

};

#endif // VIDEOSETTINGS_H
