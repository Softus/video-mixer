#ifndef DICOMSERVERSETTINGS_H
#define DICOMSERVERSETTINGS_H

#include <QWidget>

class DicomServerSettings : public QWidget
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit DicomServerSettings(QWidget *parent = 0);

signals:

public slots:
    void onClickAdd();
};

#endif // DICOMSERVERSETTINGS_H
