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

#include "mandatoryfieldssettings.h"
#include "../defaults.h"
#include <QBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>

MandatoryFieldsSettings::MandatoryFieldsSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    settings.beginGroup("ui");
    auto fieldLabels = QStringList() << tr("Patient ID")
        << tr("Name") << tr("Sex") << tr("Birthday") << tr("Physician") << tr("Study type");
    auto fieldNames = QStringList() << "PatientID"
        << "Name" << "Sex" << "Birthday" << "Physician" << "StudyType";
    auto listMandatory = settings.value("patient-data-mandatory-fields", DEFAULT_MANDATORY_FIELDS).toStringList();

    if (settings.value("patient-data-show-accession-number").toBool())
    {
        fieldLabels << tr("Accession number");
        fieldNames << "AccessionNumber";
    }

    auto layoutMain = new QVBoxLayout;
    layoutMain->setContentsMargins(4,0,4,0);

    listFields = new QListWidget();
    listFields->addItems(fieldLabels);
    for (auto i = 0; i < listFields->count(); ++i)
    {
        auto item = listFields->item(i);
        item->setCheckState(listMandatory.contains(fieldNames[i])? Qt::Checked: Qt::Unchecked);
        item->setData(Qt::UserRole, fieldNames[i]);
    }

    layoutMain->addWidget(listFields);
    setLayout(layoutMain);
}

void MandatoryFieldsSettings::save(QSettings& settings)
{
    QStringList fields;

    for (auto i = 0; i < listFields->count(); ++i)
    {
        auto item = listFields->item(i);
        if (item->checkState() != Qt::Checked)
        {
            continue;
        }
        fields.append(item->data(Qt::UserRole).toString());
    }

    settings.setValue("ui/patient-data-mandatory-fields", fields);
}
