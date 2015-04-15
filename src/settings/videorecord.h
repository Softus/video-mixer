/*
 * Copyright (C) 2013-2015 Irkutsk Diagnostic Center.
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

#ifndef VIDEORECORDSETTINGS_H
#define VIDEORECORDSETTINGS_H

#include <QWidget>
#include <QSettings>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QGroupBox;
class QSpinBox;
QT_END_NAMESPACE

class VideoRecordSettings : public QWidget
{
    Q_OBJECT
    QSpinBox  *spinCountdown;
    QCheckBox *checkLimit;
    QSpinBox  *spinNotify;
    QCheckBox *checkNotify;
    QGroupBox *grpRecordLog;
    QCheckBox *checkMaxVideoSize;
    QSpinBox  *spinMaxVideoSize;
    QGroupBox *grpMotionDetection;
    QCheckBox *checkMotionStart;
    QCheckBox *checkMotionStop;
    QCheckBox *checkMotionDebug;
    QSpinBox  *spinSensitivity;
    QSpinBox  *spinThreshold;
    QSpinBox  *spinMinTime;
    QSpinBox  *spinGap;

public:
    Q_INVOKABLE explicit VideoRecordSettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void save(QSettings& settings);
};

#endif // VIDEORECORDSETTINGS_H
