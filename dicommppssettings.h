#ifndef DICOMMPPSSETTINGS_H
#define DICOMMPPSSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
QT_END_NAMESPACE

class DicomMppsSettings : public QWidget
{
    Q_OBJECT
    QCheckBox* checkStartWithMpps;
    QCheckBox* checkCompleteWithMpps;
    QCheckBox* checkUseMpps;
    QComboBox* cbMppsServer;

public:
    Q_INVOKABLE explicit DicomMppsSettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void onUpdateServers();
    void onServerChanged(int idx);
    void save();
};

#endif // DICOMMPPSSETTINGS_H
