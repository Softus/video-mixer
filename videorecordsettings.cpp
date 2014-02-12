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
#include <QFormLayout>
#include <QSettings>
#include <QSpinBox>

VideoRecordSettings::VideoRecordSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    auto layoutMain = new QFormLayout;
    layoutMain->addRow(checkLimit = new QCheckBox(tr("&Limit clips to")), spinCountdown = new QSpinBox);
    checkLimit->setChecked(settings.value("clip-limit").toBool());
    spinCountdown->setSuffix(tr(" seconds"));
    spinCountdown->setRange(1, 3600);
    spinCountdown->setValue(settings.value("clip-countdown", DEFAULT_VIDEO_RECORD_LIMIT).toInt());
    layoutMain->addRow(checkNotify = new QCheckBox(tr("&Play a sound on")), spinNotify = new QSpinBox);
    checkNotify->setChecked(settings.value("notify-clip-limit").toBool());
    spinNotify->setSuffix(tr(" seconds before stopping"));
    spinNotify->setRange(1, 3600);
    spinNotify->setValue(settings.value("notify-clip-countdown", DEFAULT_VIDEO_RECORD_NOTIFY).toInt());

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
