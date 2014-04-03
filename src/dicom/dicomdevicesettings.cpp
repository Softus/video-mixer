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

#include "dicomdevicesettings.h"
#include "../defaults.h"
#include "../product.h"
#include <QComboBox>
#include <QCheckBox>
#include <QDebug>
#include <QFormLayout>
#include <QLineEdit>
#include <QSettings>
#include <QSpinBox>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QHostAddress>

/*
C.7.3.1.1.1 Modality
Defined Terms for the Modality (0008,0060) are:
CR = Computed Radiography
CT = Computed Tomography
MR = Magnetic Resonance
NM = Nuclear Medicine
*US = Ultrasound
    #define UID_UltrasoundImageStorage                                 "1.2.840.10008.5.1.4.1.1.6.1"
    // Doesnt't work #define UID_UltrasoundMultiframeImageStorage                       "1.2.840.10008.5.1.4.1.1.3.1"
    #define UID_RawDataStorage                                         "1.2.840.10008.5.1.4.1.1.66"

OT = Other
BI = Biomagnetic imaging
CD = Color flow Doppler
DD = Duplex Doppler
DG = Diaphanography
*ES = Endoscopy
    #define UID_VLEndoscopicImageStorage                               "1.2.840.10008.5.1.4.1.1.77.1.1"
    #define UID_VideoEndoscopicImageStorage                            "1.2.840.10008.5.1.4.1.1.77.1.1.1"
LS = Laser surface scan
PT = Positron emission tomography (PET)
RG = Radiographic imaging (conventional film/screen)
ST = Single-photon emission computed tomography (SPECT)
TG = Thermography
XA = X-Ray Angiography
RF = Radio Fluoroscopy
RTIMAGE = Radiotherapy Image
RTDOSE = Radiotherapy Dose
RTSTRUCT = Radiotherapy Structure Set
RTPLAN = Radiotherapy Plan
RTRECORD = RT Treatment Record
HC = Hard Copy
DX = Digital Radiography
MG = Mammography
IO = Intra-oral Radiography
PX = Panoramic X-Ray
*GM = General Microscopy
    #define UID_VLMicroscopicImageStorage                              "1.2.840.10008.5.1.4.1.1.77.1.2"
    #define UID_VideoMicroscopicImageStorage                           "1.2.840.10008.5.1.4.1.1.77.1.2.1"
SM = Slide Microscopy
*XC = External-camera Photography
    #define UID_VLPhotographicImageStorage                             "1.2.840.10008.5.1.4.1.1.77.1.4"
    #define UID_VideoPhotographicImageStorage                          "1.2.840.10008.5.1.4.1.1.77.1.4.1"
PR = Presentation State
AU = Audio
ECG = Electrocardiography
EPS = Cardiac Electrophysiology
HD = Hemodynamic Waveform
SR = SR Document
IVUS = Intravascular Ultrasound
OP = Ophthalmic Photography
SMR = Stereometric Relationship
OCT = Optical Coherence Tomography
OPR = Ophthalmic Refraction
OPV = Ophthalmic Visual Field
OPM = Ophthalmic Mapping
KO = Key Object Selection
SEG = Segmentation
REG = Registration */

DicomDeviceSettings::DicomDeviceSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    settings.beginGroup("dicom");
    auto mainLayout = new QFormLayout;

    auto localHost = QHostInfo::localHostName();
    auto textName = new QLineEdit(localHost);
    textName->setReadOnly(true);
    mainLayout->addRow(tr("&Name"), textName);

    auto textIp = new QLineEdit;
    textIp->setReadOnly(true);
    QString strIps;
    foreach (auto addr, QHostInfo::fromName(localHost).addresses())
    {
        if (addr.scopeId() == "Node-local" || addr.isInSubnet(QHostAddress(0x7F000000), 8))
        {
            // Skip 127.x.x.x and IPv6 local subnet
            continue;
        }
        if (!strIps.isEmpty())
        {
            strIps.append(", ");
        }
        strIps.append(addr.toString());
    }
    textIp->setText(strIps);

    mainLayout->addRow(tr("&IP address"), textIp);

    mainLayout->addRow(tr("AE &title"), textAet = new QLineEdit(settings.value("aet", localHost.toUpper()).toString()));
    mainLayout->addRow(tr("&Modality"), cbModality = new QComboBox);
    cbModality->addItem(tr("Endoscopy"), "ES");
    cbModality->addItem(tr("Ultrasound"), "US");
    cbModality->addItem(tr("General Microscopy"), "GM");
    cbModality->addItem(tr("External-camera Photography"), "XC");
    auto idx = cbModality->findData(settings.value("modality", DEFAULT_MODALITY).toString());
    cbModality->setCurrentIndex(qMax(idx, 0));
    mainLayout->addRow(tr("&Port"), spinPort = new QSpinBox);
    spinPort->setRange(0, 65535);
    spinPort->setValue(settings.value("local-port").toInt());

    mainLayout->addRow(nullptr, checkExportClips = new QCheckBox(tr("Export video &clips to DICOM")));
    checkExportClips->setChecked(settings.value("export-clips", DEFAULT_EXPORT_CLIPS_TO_DICOM).toBool());
    mainLayout->addRow(nullptr, checkExportVideo = new QCheckBox(tr("Export &video log to DICOM")));
    checkExportVideo->setChecked(settings.value("export-video", DEFAULT_EXPORT_VIDEO_TO_DICOM).toBool());
    mainLayout->addRow(nullptr, checkTransCyr = new QCheckBox(tr("DICOM server accepts 7-bit &ASCII only")));
    checkTransCyr->setChecked(settings.value("translate-cyrillic", DEFAULT_TRANSLATE_CYRILLIC).toBool());

    setLayout(mainLayout);
}

void DicomDeviceSettings::save()
{
    QSettings settings;
    settings.beginGroup("dicom");

    settings.setValue("aet", textAet->text());
    settings.setValue("modality", cbModality->itemData(cbModality->currentIndex()).toString());
    settings.setValue("local-port", spinPort->value());
    settings.setValue("export-clips", checkExportClips->isChecked());
    settings.setValue("export-video", checkExportVideo->isChecked());
    settings.setValue("translate-cyrillic", checkTransCyr->isChecked());
}
