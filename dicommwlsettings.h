#ifndef DICOMMWLSETTINGS_H
#define DICOMMWLSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
QT_END_NAMESPACE

class DicomMwlSettings : public QWidget
{
    Q_OBJECT
    QCheckBox* checkStartWithMwl;
    QCheckBox* checkUseMwl;
    QComboBox* cbMwlServer;

public:
    Q_INVOKABLE explicit DicomMwlSettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void onUpdateServers();
    void onServerChanged(int idx);
    void save();
};

#endif // DICOMMWLSETTINGS_H
