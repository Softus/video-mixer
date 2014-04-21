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

#include "dicomstoragesettings.h"
#include "../defaults.h"
#include <QBoxLayout>
#include <QCheckBox>
#include <QListWidget>
#include <QSettings>

DicomStorageSettings::DicomStorageSettings(QWidget *parent) :
    QWidget(parent)
{
    auto layoutMain = new QVBoxLayout;
    layoutMain->setContentsMargins(4,0,4,0);
    layoutMain->addWidget(listServers = new QListWidget);
    layoutMain->addWidget(checkStoreVideoAsBinary = new QCheckBox(tr("Store video files as regular binary")));
    checkStoreVideoAsBinary->setChecked(QSettings().value("dicom/store-video-as-binary", DEFAULT_STORE_VIDEO_AS_BINARY).toBool());

    setLayout(layoutMain);
}

QStringList DicomStorageSettings::checkedServers()
{
    QStringList listChecked;
    for (auto i = 0; i < listServers->count(); ++i)
    {
        auto item = listServers->item(i);
        if (item->checkState() == Qt::Checked)
        {
            listChecked << item->text();
        }
    }
    return listChecked;
}

void DicomStorageSettings::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);

    QSettings settings;
    QStringList listChecked = (listServers->count() == 0)? settings.value("dicom/storage-servers").toStringList(): checkedServers();
    QStringList serverNames = parent()->property("DICOM servers").toStringList();

    if (serverNames.isEmpty())
    {
        settings.beginGroup("servers");
        serverNames = settings.allKeys();
        settings.endGroup();
    }

    listServers->clear();
    listServers->addItems(serverNames);

    for (auto i = 0; i < listServers->count(); ++i)
    {
        auto item = listServers->item(i);
        item->setCheckState(listChecked.contains(item->text())? Qt::Checked: Qt::Unchecked);
    }
}

void DicomStorageSettings::save(QSettings& settings)
{
    settings.beginGroup("dicom");
    settings.setValue("storage-servers", checkedServers());
    settings.setValue("store-video-as-binary", checkStoreVideoAsBinary->isChecked());
    settings.endGroup();
}
