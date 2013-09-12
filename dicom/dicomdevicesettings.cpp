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

#include "dicomdevicesettings.h"
#include <QCheckBox>
#include <QDebug>
#include <QFormLayout>
#include <QLineEdit>
#include <QSettings>
#include <QSpinBox>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QHostAddress>

DicomDeviceSettings::DicomDeviceSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
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
    mainLayout->addRow(tr("&Port"), spinPort = new QSpinBox);
    spinPort->setRange(0, 65535);
    spinPort->setValue(settings.value("local-port").toInt());

    mainLayout->addRow(nullptr, checkExport = new QCheckBox(tr("&Export video to DICOM")));
    checkExport->setChecked(settings.value("dicom-export", true).toBool());
    setLayout(mainLayout);
}

void DicomDeviceSettings::save()
{
    QSettings settings;

    settings.setValue("aet", textAet->text());
    settings.setValue("local-port", spinPort->value());
    settings.setValue("dicom-export", checkExport->isChecked());
}
