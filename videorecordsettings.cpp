#include "videorecordsettings.h"
#include "defaults.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QSettings>
#include <QSpinBox>

VideoRecordSettings::VideoRecordSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    auto layoutMain = new QFormLayout;
    layoutMain->addRow(checkLimit = new QCheckBox(tr("&Limit clips to")), spinCountdown = new QSpinBox);
    checkLimit->setChecked(settings.value("clip-limit", DEFAULT_VIDEO_RECORD_LIMIT).toBool());
    spinCountdown->setSuffix(tr(" seconds"));
    spinCountdown->setRange(1, 3600);
    spinCountdown->setValue(settings.value("clip-countdown").toInt());
    layoutMain->addRow(checkNotify = new QCheckBox(tr("&Play a sound on")), spinNotify = new QSpinBox);
    checkNotify->setChecked(settings.value("notify-clip-limit", DEFAULT_VIDEO_RECORD_NOTIFY).toBool());
    spinNotify->setSuffix(tr(" seconds before stopping"));
    spinNotify->setRange(1, 3600);
    spinNotify->setValue(settings.value("notify-clip-countdown").toInt());

    setLayout(layoutMain);
}

void VideoRecordSettings::save()
{
    QSettings settings;

    settings.setValue("clip-limit",  checkLimit->isChecked());
    settings.setValue("clip-countdown",  spinCountdown->value());
    settings.setValue("notify-clip-limit",  checkNotify->isChecked());
    settings.setValue("notify-clip-countdown",  spinNotify->value());
}
