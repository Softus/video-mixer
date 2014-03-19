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
#include "../defaults.h"

#include <QCheckBox>
#include <QBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QSettings>
#include <QSpinBox>

#include <QGst/ElementFactory>

VideoRecordSettings::VideoRecordSettings(QWidget *parent)
    : QWidget(parent)
    , checkMaxVideoSize(nullptr)
    , spinMaxVideoSize(nullptr)
{
    QSettings settings;
    auto layoutMain = new QVBoxLayout;
    layoutMain->setContentsMargins(4,0,4,0);

    auto grpClips = new QGroupBox(tr("Video clips"));
    auto layoutClips = new QFormLayout;
    layoutClips->addRow(checkLimit = new QCheckBox(tr("&Limit clip duration at")), spinCountdown = new QSpinBox);
    checkLimit->setChecked(settings.value("clip-limit", DEFAULT_CLIP_LIMIT).toBool());
    spinCountdown->setSuffix(tr(" seconds"));
    spinCountdown->setRange(1, 3600);
    spinCountdown->setValue(settings.value("clip-countdown", DEFAULT_VIDEO_RECORD_LIMIT).toInt());
    layoutClips->addRow(checkNotify = new QCheckBox(tr("&Play alert at")), spinNotify = new QSpinBox);
    checkNotify->setChecked(settings.value("notify-clip-limit", DEFAULT_NOTIFY_CLIP_LIMIT).toBool());
    spinNotify->setSuffix(tr(" seconds till stop"));
    spinNotify->setRange(1, 3600);
    spinNotify->setValue(settings.value("notify-clip-countdown", DEFAULT_VIDEO_RECORD_NOTIFY).toInt());
    grpClips->setLayout(layoutClips);
    layoutMain->addWidget(grpClips);

    layoutMain->addWidget(grpRecordLog = new QGroupBox(tr("Record &video log")));
    auto layoutLog = new QFormLayout;
    grpRecordLog->setCheckable(true);
    grpRecordLog->setChecked(settings.value("enable-video").toBool());
    grpRecordLog->setLayout(layoutLog);

    auto elm = QGst::ElementFactory::make("multifilesink");
    if (elm && elm->findProperty("max-file-size"))
    {
        layoutLog->addRow(checkMaxVideoSize = new QCheckBox(tr("&Split files by")), spinMaxVideoSize = new QSpinBox);
        connect(checkMaxVideoSize, SIGNAL(toggled(bool)), spinMaxVideoSize, SLOT(setEnabled(bool)));
        checkMaxVideoSize->setChecked(settings.value("split-video-files", DEFAULT_SPLIT_VIDEO_FILES).toBool());

        spinMaxVideoSize->setSuffix(tr(" Mb"));
        spinMaxVideoSize->setRange(1, 1024*1024);
        spinMaxVideoSize->setValue(settings.value("video-max-file-size", DEFAULT_VIDEO_MAX_FILE_SIZE).toInt());
        spinMaxVideoSize->setEnabled(checkMaxVideoSize->isChecked());
    }

    grpMotionDetection = new QGroupBox(tr("Use &motion detection"));
    grpMotionDetection->setCheckable(true);
    grpMotionDetection->setChecked(settings.value("detect-motion", DEFAULT_MOTION_DETECTION).toBool());
    auto layoutMotionDetection = new QFormLayout;
    layoutMotionDetection->setContentsMargins(0,8,0,8);

    layoutMotionDetection->addRow(checkMotionStart = new QCheckBox(tr("St&art after")), spinMinTime = new QSpinBox);
    checkMotionStart->setChecked(settings.value("motion-start", true).toBool());
    connect(checkMotionStart, SIGNAL(toggled(bool)), spinMinTime, SLOT(setEnabled(bool)));
    spinMinTime->setValue(settings.value("motion-min-frames", DEFAULT_MOTION_MIN_FRAMES).toInt());
    spinMinTime->setSuffix(tr(" frames with motion"));
    spinMinTime->setEnabled(checkMotionStart->isChecked());

    layoutMotionDetection->addRow(checkMotionStop = new QCheckBox(tr("St&op after")), spinGap = new QSpinBox);
    checkMotionStop->setChecked(settings.value("motion-stop", true).toBool());
    connect(checkMotionStop, SIGNAL(toggled(bool)), spinGap, SLOT(setEnabled(bool)));
    spinGap->setValue(settings.value("motion-gap", DEFAULT_MOTION_GAP).toInt());
    spinGap->setSuffix(tr(" seconds without motion"));
    spinGap->setEnabled(checkMotionStop->isChecked());

    layoutMotionDetection->addRow(tr("S&ensitivity"), spinSensitivity = new QSpinBox);
    spinSensitivity->setValue(settings.value("motion-sensitivity", DEFAULT_MOTION_SENSITIVITY).toReal()* 100);
    spinSensitivity->setSuffix(tr("%"));

    layoutMotionDetection->addRow(tr("&Threshold"), spinThreshold = new QSpinBox);
    spinThreshold->setValue(settings.value("motion-threshold", DEFAULT_MOTION_THRESHOLD).toReal()* 100);
    spinThreshold->setSuffix(tr("%"));

    layoutMotionDetection->addRow(nullptr, checkMotionDebug = new QCheckBox(tr("&Highlight areas with motion")));
    checkMotionDebug->setChecked(settings.value("motion-debug").toBool());
    grpMotionDetection->setLayout(layoutMotionDetection);
    layoutLog->addRow(grpMotionDetection);

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

    if (spinMaxVideoSize)
    {
        settings.setValue("split-video-files", checkMaxVideoSize->isChecked());
        settings.setValue("video-max-file-size", spinMaxVideoSize->value());
    }

    settings.setValue("clip-limit",             checkLimit->isChecked());
    settings.setValue("clip-countdown",         spinCountdown->value());
    settings.setValue("notify-clip-limit",      checkNotify->isChecked());
    settings.setValue("notify-clip-countdown",  spinNotify->value());
    settings.setValue("enable-video",           grpRecordLog->isChecked());
    settings.setValue("detect-motion",          useDetection);
    settings.setValue("motion-start",           checkMotionStart->isChecked());
    settings.setValue("motion-stop",            checkMotionStop->isChecked());
    settings.setValue("motion-sensitivity",     spinSensitivity->value() * 0.01);
    settings.setValue("motion-threshold",       spinThreshold->value() * 0.01);
    settings.setValue("motion-min-frames",      spinMinTime->value());
    settings.setValue("motion-gap",             spinGap->value());
    settings.setValue("motion-debug",           checkMotionDebug->isChecked());
}
