#include "rtpsettings.h"
#include <QCheckBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSettings>

RtpSettings::RtpSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;

    QFormLayout* layout = new QFormLayout;
    textRtpClients = new QLineEdit(settings.value("rtp-clients").toString());
    layout->addRow(tr("&RTP clients"), textRtpClients);
    checkEnableRtp = new QCheckBox(tr("&Enable RTP"));
    checkEnableRtp->setChecked(settings.value("enable-rtp").toBool());
    layout->addRow(nullptr, checkEnableRtp);

    setLayout(layout);
}

void RtpSettings::save()
{
    QSettings settings;
    settings.setValue("enable-rtp", checkEnableRtp->isChecked());
    settings.setValue("rtp-clients", textRtpClients->text());
    //settings.setValue("rtp-sink", cbRtpSink->text());
}
