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

#include "worklistquerysettings.h"
#include "../defaults.h"
#include <QBoxLayout>
#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>

WorklistQuerySettings::WorklistQuerySettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    settings.beginGroup("dicom");
    auto scheduledDate = settings.value("worklist-by-date", DEFAULT_WORKLIST_DATE_RANGE).toInt();

    QFrame *frameDate = new QFrame();
    frameDate->setFrameShape(QFrame::Box);
    frameDate->setFrameShadow(QFrame::Sunken);
    QVBoxLayout* layoutDate = new QVBoxLayout;
    layoutDate->addWidget(radioToday = new QRadioButton(tr("&Today")));
    radioToday->setChecked(scheduledDate == 1);

    QHBoxLayout* layoutToday = new QHBoxLayout;
    layoutToday->addWidget(radioTodayDelta = new QRadioButton(tr("To&day +/-")));
    radioTodayDelta->setChecked(scheduledDate == 2);
    spinDelta = new QSpinBox;
    spinDelta->setMaximum(365*10);
    spinDelta->setSuffix(tr(" day(s)"));
    spinDelta->setValue(settings.value("worklist-delta", DEFAULT_WORKLIST_DAY_DELTA).toInt());
    layoutToday->addWidget(spinDelta);
    layoutToday->addStretch(1);
    layoutDate->addLayout(layoutToday);

    QHBoxLayout* layoutRange = new QHBoxLayout;
    layoutRange->addWidget(radioRange = new QRadioButton(tr("&From")));
    radioRange->setChecked(scheduledDate == 3);
    dateFrom = new QDateEdit;
    dateFrom->setCalendarPopup(true);
    dateFrom->setDisplayFormat(tr("MM/dd/yyyy"));
    dateFrom->setDate(settings.value("worklist-from").toDate());
    layoutRange->addWidget(dateFrom);
    dateTo = new QDateEdit;
    dateTo->setCalendarPopup(true);
    dateTo->setDisplayFormat(tr("MM/dd/yyyy"));
    dateTo->setDate(settings.value("worklist-to").toDate());
    QLabel* lblTo = new QLabel(tr("t&o"));
    lblTo->setBuddy(dateTo);
    layoutRange->addWidget(lblTo);
    layoutRange->addWidget(dateTo);
    layoutRange->addStretch(1);
    layoutDate->addLayout(layoutRange);
    frameDate->setLayout(layoutDate);

    QFormLayout* layoutMain = new QFormLayout;
    layoutMain->addRow(checkScheduledDate = new QCheckBox(tr("&Scheduled\ndate")), frameDate);
    checkScheduledDate->setChecked(scheduledDate > 0);

    cbAeTitle = new QComboBox;
    cbAeTitle->setEditable(true);
    checkAeTitle = new QCheckBox(tr("&AE title"));
    auto aet = settings.value("worklist-aet").toString();
    if (!aet.isEmpty())
    {
        checkAeTitle->setChecked(true);
        cbAeTitle->addItem(aet);
        cbAeTitle->setCurrentIndex(0);
    }
    layoutMain->addRow(checkAeTitle, cbAeTitle);

    cbModality = new QComboBox;
    cbModality->setEditable(true);
    checkModality = new QCheckBox(tr("&Modality"));
    auto modality = settings.value("worklist-modality").toString();
    if (!modality.isEmpty())
    {
        checkModality->setChecked(true);
        cbModality->addItem(modality);
        cbModality->setCurrentIndex(0);
    }
    layoutMain->addRow(checkModality, cbModality);

    setLayout(layoutMain);
}

void WorklistQuerySettings::save(QSettings& settings)
{
    // This should be an enum definetelly
    //
    auto scheduledDate = !checkScheduledDate->isChecked()? 0:
         radioToday->isChecked()? 1:
         radioTodayDelta->isChecked()? 2:
         3;

    settings.beginGroup("dicom");
    settings.setValue("worklist-by-date", scheduledDate);
    settings.setValue("worklist-delta", spinDelta->value());
    settings.setValue("worklist-from", dateFrom->date());
    settings.setValue("worklist-to", dateTo->date());
    settings.setValue("worklist-modality", checkModality->isChecked()? cbModality->currentText(): nullptr);
    settings.setValue("worklist-aet", checkAeTitle->isChecked()? cbAeTitle->currentText(): nullptr);
    settings.endGroup();
}
