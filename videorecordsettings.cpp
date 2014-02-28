/*
 * Copyright (C) 2013-2014 Irkutsk Diagnostic Center.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "videorecordsettings.h"
#include "defaults.h"

#include <QCheckBox>
#include <QBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QSettings>
#include <QSpinBox>

#include <QGst/ElementFactory>

VideoRecordSettings::VideoRecordSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    auto layoutMain = new QVBoxLayout;
    layoutMain->setContentsMargins(4,0,4,0);

    auto grpClips = new QGroupBox(tr("Video clips"));
    auto layoutClips = new QFormLayout;
    layoutClips->addRow(checkLimit = new QCheckBox(tr("&Limit clips to")), spinCountdown = new QSpinBox);
    checkLimit->setChecked(settings.value("clip-limit", DEFAULT_CLIP_LIMIT).toBool());
    spinCountdown->setSuffix(tr(" seconds"));
    spinCountdown->setRange(1, 3600);
    spinCountdown->setValue(settings.value("clip-countdown", DEFAULT_VIDEO_RECORD_LIMIT).toInt());
    layoutClips->addRow(checkNotify = new QCheckBox(tr("&Play a sound on")), spinNotify = new QSpinBox);
    checkNotify->setChecked(settings.value("notify-clip-limit", DEFAULT_NOTIFY_CLIP_LIMIT).toBool());
    spinNotify->setSuffix(tr(" seconds before stopping"));
    spinNotify->setRange(1, 3600);
    spinNotify->setValue(settings.value("notify-clip-countdown", DEFAULT_VIDEO_RECORD_NOTIFY).toInt());
    grpClips->setLayout(layoutClips);
    layoutMain->addWidget(grpClips);

    layoutMain->addWidget(checkRecordLog = new QCheckBox(tr("Record &video log")));
    checkRecordLog->setChecked(settings.value("enable-video").toBool());

    grpMotionDetection = new QGroupBox(tr("Use &motion detection"));
    layoutClips->addWidget(grpMotionDetection);
    grpMotionDetection->setCheckable(true);
    grpMotionDetection->setChecked(settings.value("detect-motion", DEFAULT_MOTION_DETECTION).toBool());
    auto layoutVideoLog = new QFormLayout;
    layoutVideoLog->addRow(nullptr, checkMotionStart = new QCheckBox(tr("&Start recording only after the motion is detected")));
    checkMotionStart->setChecked(settings.value("motion-start", true).toBool());
    layoutVideoLog->addRow(nullptr, checkMotionStop = new QCheckBox(tr("S&top recording when the motion is lost")));
    checkMotionStop->setChecked(settings.value("motion-stop", true).toBool());

    layoutVideoLog->addRow(tr("S&ensitivity"), spinSensitivity = new QSpinBox);
    spinSensitivity->setValue(settings.value("motion-sensitivity", DEFAULT_MOTION_SENSITIVITY).toReal()* 100);
    spinSensitivity->setSuffix(tr(" percent"));

    layoutVideoLog->addRow(tr("T&hreshold"), spinThreshold = new QSpinBox);
    spinThreshold->setValue(settings.value("motion-threshold", DEFAULT_MOTION_THRESHOLD).toReal()* 100);
    spinThreshold->setSuffix(tr(" percent"));

    layoutVideoLog->addRow(tr("T&rigger after"), spinMinTime = new QSpinBox);
    spinMinTime->setValue(settings.value("motion-min-frames", DEFAULT_MOTION_MIN_FRAMES).toInt());
    spinMinTime->setSuffix(tr(" frames"));

    layoutVideoLog->addRow(nullptr, checkMotionDebug = new QCheckBox(tr("&Display motion cells")));
    checkMotionDebug->setChecked(settings.value("motion-debug").toBool());
    grpMotionDetection->setLayout(layoutVideoLog);
    layoutMain->addWidget(grpMotionDetection);

    layoutMain->addStretch();

    setLayout(layoutMain);
}

void VideoRecordSettings::save()
{
    QSettings settings;

    auto useDetection = grpMotionDetection->isChecked();
    if (useDetection)
    {
        auto elm = QGst::ElementFactory::make("motioncells");
        if (elm.isNull() && QMessageBox::question(this, windowTitle()
          , tr("Motion detection requires the opencv plugin which is not found.\nAre you want to continue?")
          , QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
        {
            useDetection = false;
        }
    }

    settings.setValue("clip-limit",             checkLimit->isChecked());
    settings.setValue("clip-countdown",         spinCountdown->value());
    settings.setValue("notify-clip-limit",      checkNotify->isChecked());
    settings.setValue("notify-clip-countdown",  spinNotify->value());
    settings.setValue("enable-video",           checkRecordLog->isChecked());
    settings.setValue("detect-motion",          useDetection);
    settings.setValue("motion-start",           checkMotionStart->isChecked());
    settings.setValue("motion-stop",            checkMotionStop->isChecked());
    settings.setValue("motion-sensitivity",     spinSensitivity->value() * 0.01);
    settings.setValue("motion-threshold",       spinThreshold->value() * 0.01);
    settings.setValue("motion-min-frames",      spinMinTime->value());
    settings.setValue("motion-debug",           checkMotionDebug->isChecked());
}
