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

#include <QDialog>
#include <QSettings>
#include <QtGlobal>

#include <QGst/Caps>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QTextEdit;
class QxtLineEdit;
QT_END_NAMESPACE

#define DEFAULT_VIDEOBITRATE 4000

class VideoSourceDetails : public QDialog
{
    Q_OBJECT
    QLineEdit *editAlias;
#ifdef WITH_DICOM
    QxtLineEdit *editModality;
#endif
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
    QLineEdit *editRtpClients;
    QCheckBox *checkEnableRtp;
    QLineEdit *editHttpPushUrl;
    QCheckBox *checkEnableHttp;
    QCheckBox *checkDeinterlace;
    QGst::CapsPtr caps;

    QVariant selectedChannel;
    QString  selectedFormat;
    QSize    selectedSize;

    QString updateGstList(const QVariantMap& parameters, const char* settingName, const char* def,
                          unsigned long long type, QComboBox* cb);

public:
    explicit VideoSourceDetails(const QVariantMap& parameters, QWidget *parent = 0);
    void updateDevice(const QString& device, const QString& deviceType);
    void updateParameters(QVariantMap& parameters);

signals:
    
private slots:
    void inputChannelChanged(int index);
    void formatChanged(int index);
};

#endif // VIDEOSETTINGS_H
