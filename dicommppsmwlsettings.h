#ifndef DICOMMPPSMWLSETTINGS_H
#define DICOMMPPSMWLSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
QT_END_NAMESPACE

class DicomMppsMwlSettings : public QWidget
{
    Q_OBJECT
    QCheckBox* checkUseMwl;
    QComboBox* cbMwlServer;
    QCheckBox* checkTransCyr;

    QCheckBox* checkStartWithMpps;
    QCheckBox* checkCompleteWithMpps;
    QCheckBox* checkUseMpps;
    QComboBox* cbMppsServer;

public:
    Q_INVOKABLE explicit DicomMppsMwlSettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void onUpdateServers();
    void onUseToggle(bool checked);
    void save();
};

#endif // DICOMMPPSMWLSETTINGS_H
