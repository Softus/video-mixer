#ifndef VIDEORECORDSETTINGS_H
#define VIDEORECORDSETTINGS_H

#include <QWidget>

class QCheckBox;
class QSpinBox;

class VideoRecordSettings : public QWidget
{
    Q_OBJECT
    QSpinBox *spinCountdown;
    QCheckBox *checkLimit;
    QSpinBox *spinNotify;
    QCheckBox *checkNotify;

public:
    Q_INVOKABLE explicit VideoRecordSettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void save();
};

#endif // VIDEORECORDSETTINGS_H
