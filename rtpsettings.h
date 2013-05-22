#ifndef RTPSETTINGS_H
#define RTPSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
QT_END_NAMESPACE

class RtpSettings : public QWidget
{
    Q_OBJECT

    QLineEdit* textRtpClients;
    QCheckBox* checkEnableRtp;

public:
    Q_INVOKABLE explicit RtpSettings(QWidget *parent = 0);

signals:

public slots:
    void save();
};

#endif // RTPSETTINGS_H
