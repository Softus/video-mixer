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

#include "patientdialog.h"
#include "product.h"
#include "defaults.h"
#include "mandatoryfieldgroup.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QDateEdit>
#include <QDBusInterface>
#include <QDebug>
#include <QFormLayout>
#include <QxtLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QSettings>

StartStudyDialog::StartStudyDialog(QWidget *parent) :
    QDialog(parent)
{
    QSettings settings;
    auto listMandatory = settings.value("new-study-mandatory-fields", DEFAULT_MANDATORY_FIELDS).toStringList();

    setWindowTitle(tr("New study"));
    setMinimumSize(480, 240);

    auto layoutMain = new QFormLayout;
    layoutMain->addRow(tr("&Patient ID"), textPatientId = new QxtLineEdit);
    layoutMain->addRow(tr("&Name"), textPatientName = new QxtLineEdit);
    layoutMain->addRow(tr("&Sex"), cbPatientSex = new QComboBox);
    cbPatientSex->setLineEdit(new QxtLineEdit);
    cbPatientSex->addItems(QStringList() << "" << tr("female") << tr("male") << tr("other"));
    QChar patientSexCodes[] = {'U', 'F', 'M', 'O'};
    for (int i = 0; i < cbPatientSex->count(); ++i)
    {
        cbPatientSex->setItemData(i, patientSexCodes[i]);
    }
    cbPatientSex->setEditable(true);

    layoutMain->addRow(tr("&Birthday"), dateBirthday = new QDateEdit);
    dateBirthday->setCalendarPopup(true);
    dateBirthday->setDisplayFormat(tr("MM/dd/yyyy"));

    layoutMain->addRow(tr("P&hysician"), cbPhysician = new QComboBox);
    cbPhysician->setLineEdit(new QxtLineEdit);
    cbPhysician->addItems(QSettings().value("physicians").toStringList());
    cbPhysician->setCurrentIndex(0); // Select first, if any
    cbPhysician->setEditable(true);

    layoutMain->addRow(tr("Study &type"), cbStudyType = new QComboBox);
    cbStudyType->setLineEdit(new QxtLineEdit);
    cbStudyType->addItems(QSettings().value("studies").toStringList());
    cbStudyType->setCurrentIndex(0); // Select first, if any
    cbStudyType->setEditable(true);

    auto layoutBtns = new QHBoxLayout;

#ifdef WITH_DICOM
    auto btnWorklist = new QPushButton(QIcon(":buttons/show_worklist"), nullptr);
    btnWorklist->setToolTip(tr("Show work list"));
    connect(btnWorklist, SIGNAL(clicked()), this, SLOT(onShowWorklist()));
    layoutBtns->addWidget(btnWorklist);
    btnWorklist->setEnabled(!settings.value("mwl-server").toString().isEmpty());
#endif

    layoutBtns->addStretch(1);

    auto btnReject = new QPushButton(tr("Cancel"));
    connect(btnReject, SIGNAL(clicked()), this, SLOT(reject()));
    layoutBtns->addWidget(btnReject);

    auto btnStart = new QPushButton(tr("Start study"));
    connect(btnStart, SIGNAL(clicked()), this, SLOT(accept()));
    btnStart->setDefault(true);
    layoutBtns->addWidget(btnStart);

    layoutMain->addRow(layoutBtns);

    setLayout(layoutMain);
    restoreGeometry(settings.value("new-patient-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("new-patient-state").toInt());

    if (!listMandatory.isEmpty())
    {
        auto group = new MandatoryFieldGroup(this);
        if (listMandatory.contains("PatientID"))
            group->add(textPatientId);
        if (listMandatory.contains("Name"))
            group->add(textPatientName);
        if (listMandatory.contains("Sex"))
            group->add(cbPatientSex);
        if (listMandatory.contains("Birthday"))
            group->add(dateBirthday);
        if (listMandatory.contains("Physician"))
            group->add(cbPhysician);
        if (listMandatory.contains("StudyType"))
            group->add(cbStudyType);
        group->setOkButton(btnStart);
    }
}

void StartStudyDialog::showEvent(QShowEvent *)
{
    QSettings settings;
    if (settings.value("show-onboard").toBool())
    {
        QDBusInterface("org.onboard.Onboard", "/org/onboard/Onboard/Keyboard",
                       "org.onboard.Onboard.Keyboard").call( "Show");
    }
}

void StartStudyDialog::hideEvent(QHideEvent *)
{
    QSettings settings;
    settings.setValue("new-patient-geometry", saveGeometry());
    settings.setValue("new-patient-state", (int)windowState() & ~Qt::WindowMinimized);

    if (settings.value("show-onboard").toBool())
    {
        QDBusInterface("org.onboard.Onboard", "/org/onboard/Onboard/Keyboard",
                       "org.onboard.Onboard.Keyboard").call( "Hide");
    }
}

QString StartStudyDialog::patientId() const
{
    return textPatientId->text();
}

QString StartStudyDialog::patientName() const
{
    return textPatientName->text();
}

QDate StartStudyDialog::patientBirthDate() const
{
    return dateBirthday->date();
}

QString StartStudyDialog::patientBirthDateStr() const
{
    return patientBirthDate().toString("yyyyMMdd");
}

QString StartStudyDialog::patientSex() const
{
    return cbPatientSex->currentText();
}

QChar StartStudyDialog::patientSexCode() const
{
    auto idx = cbPatientSex->currentIndex();
    return idx < 0? '\x0': cbPatientSex->itemData(idx).toChar();
}

QString StartStudyDialog::studyName() const
{
    return cbStudyType->currentText();
}

QString StartStudyDialog::physician() const
{
    return cbPhysician->currentText();
}

void StartStudyDialog::setPatientId(const QString& id)
{
    textPatientId->setText(id);
}

void StartStudyDialog::setPatientName(const QString& name)
{
    textPatientName->setText(name);
}

void StartStudyDialog::setPatientSex(const QString& sex)
{
    // For 'Female' search for text, for 'F' search for data
    //
    auto idx = sex.length() != 1? cbPatientSex->findText(sex): cbPatientSex->findData(sex[0].toUpper());

    if (idx < 0)
    {
        cbPatientSex->setEditText(sex);
    }
    else
    {
        cbPatientSex->setCurrentIndex(idx);
    }
}

void StartStudyDialog::setPatientBirthDate(const QDate& date)
{
    dateBirthday->setDate(date);
}

void StartStudyDialog::setPatientBirthDateStr(const QString& dateStr)
{
    setPatientBirthDate(QDate::fromString(dateStr, "yyyyMMdd"));
}

void StartStudyDialog::setStudyName(const QString& name)
{
    auto idx = cbStudyType->findText(name);
    cbStudyType->setCurrentIndex(idx);

    if (idx < 0)
    {
        cbStudyType->setEditText(name);
    }
}

void StartStudyDialog::setPhysician(const QString& name)
{
    auto idx = cbPhysician->findText(name);
    cbPhysician->setCurrentIndex(idx);

    if (idx < 0)
    {
        cbPhysician->setEditText(name);
    }
}

inline static void setEditableCb(QComboBox* cb, bool editable)
{
    if (!editable)
    {
        auto text = cb->currentText();
        cb->clear();
        cb->addItem(text);
        cb->setCurrentIndex(0);
    }
    cb->setEditable(editable);
}

void StartStudyDialog::savePatientFile(const QString& outputPath)
{
    QSettings settings(outputPath, QSettings::IniFormat);

    settings.beginGroup(PRODUCT_SHORT_NAME);
    settings.setValue("patient-id", textPatientId->text());
    settings.setValue("name", textPatientName->text());
    settings.setValue("sex", cbPatientSex->currentText());
    settings.setValue("birthday", dateBirthday->text());
    settings.setValue("physician", cbPhysician->currentText());
    settings.setValue("study", cbStudyType->currentText());
    settings.endGroup();
}

void StartStudyDialog::onShowWorklist()
{
    done(SHOW_WORKLIST_RESULT);
}
