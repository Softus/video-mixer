#ifndef DICOMSTORAGESETTINGS_H
#define DICOMSTORAGESETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

class DicomStorageSettings : public QWidget
{
    Q_OBJECT
    QListWidget* listServers;

    QStringList  checkedServers();
public:
    Q_INVOKABLE explicit DicomStorageSettings(QWidget *parent = 0);

protected:
    virtual void showEvent(QShowEvent *);

signals:
    
public slots:
    void save();
};

#endif // DICOMSTORAGESETTINGS_H
