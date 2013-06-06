#ifndef DICOMDEVICESETTINGS_H
#define DICOMDEVICESETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QSpinBox;
QT_END_NAMESPACE

class DicomDeviceSettings : public QWidget
{
    Q_OBJECT
    QLineEdit*    textAet;
    QSpinBox*     spinPort;
    QCheckBox*    checkExport;

public:
    Q_INVOKABLE explicit DicomDeviceSettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void save();

};

#endif // DICOMDEVICESETTINGS_H
