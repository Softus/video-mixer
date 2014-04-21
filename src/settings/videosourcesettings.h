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

#ifndef VIDEOSETTINGS_H
#define VIDEOSETTINGS_H

#include <QWidget>
#include <QSettings>
#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QTextEdit;
QT_END_NAMESPACE

#if defined (Q_OS_WIN)
#define PLATFORM_SPECIFIC_SOURCE "dshowvideosrc"
#define PLATFORM_SPECIFIC_PROPERTY "device-name"
#elif defined (Q_OS_UNIX)
#define PLATFORM_SPECIFIC_SOURCE "v4l2src"
#define PLATFORM_SPECIFIC_PROPERTY "device"
#elif defined (Q_OS_DARWIN)
#define PLATFORM_SPECIFIC_SOURCE "osxvideosrc"
#define PLATFORM_SPECIFIC_PROPERTY "device"
#else
#error The platform is not supported.
#endif

#define DEFAULT_VIDEOBITRATE 4000

class VideoSourceSettings : public QWidget
{
    Q_OBJECT
    QComboBox *listDevices;
    QComboBox *listChannels;
    QComboBox *listFormats;
    QComboBox *listSizes;
    QComboBox *listVideoCodecs;
    QComboBox *listVideoMuxers;
    QComboBox *listRtpPayloaders;
    QComboBox *listImageCodecs;
    QCheckBox *checkFps;
    QSpinBox  *spinFps;
    QSpinBox  *spinBitrate;
    QLineEdit *textRtpClients;
    QCheckBox *checkEnableRtp;
    QCheckBox *checkDeinterlace;

    void updateDeviceList(const char *elmName, const char *propName);
    QString updateGstList(const char* setting, const char* def, unsigned long long type, QComboBox* cb);

public:
    Q_INVOKABLE explicit VideoSourceSettings(QWidget *parent = 0);

protected:
    virtual void showEvent(QShowEvent *);

signals:
    
private slots:
    void videoDeviceChanged(int index);
    void inputChannelChanged(int index);
    void formatChanged(int index);

public slots:
    void save(QSettings& settings);
};

#endif // VIDEOSETTINGS_H
