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

#include "worklistcolumnsettings.h"
#include "../defaults.h"

#include <QBoxLayout>
#include <QTableWidget>
#include <QSettings>

#define HAVE_CONFIG_H
#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dctag.h>

static DcmTagKey rowTags[] =
{
    DCM_PatientID,
    DCM_PatientName,
    DCM_PatientBirthDate,
    DCM_PatientSex,
    DCM_PatientSize,
    DCM_PatientWeight,
    DCM_ScheduledProcedureStepDescription,
    DCM_ScheduledProcedureStepStartDate,
    DCM_ScheduledProcedureStepStartTime,
    DCM_ScheduledPerformingPhysicianName,
    DCM_ScheduledProcedureStepStatus,
};

WorklistColumnSettings::WorklistColumnSettings(QWidget *parent) :
    QWidget(parent)
{
    const QString columnNames[] =
    {
        tr("Code"),
        tr("Tag name"),
        tr("VR"),
    };

    auto layouMain = new QVBoxLayout;
    layouMain->setContentsMargins(4,0,4,0);
    layouMain->addWidget(listColumns = new QTableWidget);
    setLayout(layouMain);

    size_t columns = sizeof(columnNames)/sizeof(columnNames[0]);
    listColumns->setColumnCount(columns);
    for (size_t col = 0; col < columns; ++col)
    {
        listColumns->setHorizontalHeaderItem(col, new QTableWidgetItem(columnNames[col]));
    }

    QSettings settings;
    QStringList listChecked = settings.value("dicom/worklist-columns").toStringList();
    if (listChecked.size() == 0)
    {
        // Defaults are id, name, bithday, sex, procedure description, date, time
        listChecked = DEFAULT_WORKLIST_COLUMNS;
    }

    size_t rows = sizeof(rowTags)/sizeof(rowTags[0]);
    listColumns->setUpdatesEnabled(false);
    listColumns->setRowCount(rows);
    for (size_t row = 0; row < rows; ++row)
    {
        auto tag = DcmTag(rowTags[row]);
        auto text = QString("%1,%2").arg((ushort)tag.getGroup(), (int)4, (int)16, QChar('0')).arg((ushort)tag.getElement(), (int)4, (int)16, QChar('0'));
        auto item = new QTableWidgetItem(text);
        item->setCheckState(listChecked.contains(text)? Qt::Checked: Qt::Unchecked);
        listColumns->setItem(row, 0, item);
        listColumns->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(tag.getTagName())));
        listColumns->setItem(row, 2, new QTableWidgetItem(QString::fromUtf8(tag.getVRName())));
    }
    listColumns->resizeColumnsToContents();
    listColumns->setUpdatesEnabled(true);

    listColumns->setSelectionBehavior(QAbstractItemView::SelectRows);
    listColumns->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

QStringList WorklistColumnSettings::checkedColumns()
{
    QStringList listChecked;
    for (auto i = 0; i < listColumns->rowCount(); ++i)
    {
        auto item = listColumns->item(i, 0);
        if (item->checkState() == Qt::Checked)
        {
            listChecked << item->text();
        }
    }
    return listChecked;
}

void WorklistColumnSettings::save()
{
    QSettings settings;
    settings.setValue("dicom/worklist-columns", checkedColumns());
}
