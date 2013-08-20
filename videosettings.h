/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
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
#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QTextEdit;
QT_END_NAMESPACE

#if defined (Q_WS_WIN)
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

class VideoSettings : public QWidget
{
    Q_OBJECT
    QComboBox* listDevices;
    QComboBox* listFormats;
    QComboBox* listSizes;
    QComboBox* listVideoCodecs;
    QComboBox* listVideoMuxers;
    QComboBox* listRtpPayloaders;
    QComboBox* listImageCodecs;
    QCheckBox* checkRecordAll;
    QSpinBox* spinBitrate;
    QLineEdit* textRtpClients;
    QCheckBox* checkEnableRtp;
    QCheckBox* checkDeinterlace;

    void updateDeviceList();
    void updateGstList(const char* setting, const char* def, unsigned long long type, QComboBox* cb);

public:
    Q_INVOKABLE explicit VideoSettings(QWidget *parent = 0);

protected:
    virtual void showEvent(QShowEvent *);

signals:
    
public slots:
    void videoDeviceChanged(int index);
    void formatChanged(int index);
    void save();
};

#endif // VIDEOSETTINGS_H
