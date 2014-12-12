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

#include "dicomserverdetails.h"
#include "dcmclient.h"
#include "../defaults.h"
#include "../qwaitcursor.h"
#include <dcmtk/dcmdata/dcuid.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>

DicomServerDetails::DicomServerDetails(QWidget *parent) :
    QDialog(parent)
{
    auto layoutMain = new QFormLayout;
    layoutMain->addRow(tr("&Name"), textName = new QLineEdit);
    connect(textName, SIGNAL(editingFinished()), this, SLOT(onNameChanged()));
    layoutMain->addRow(tr("&AE title"), textAet = new QLineEdit);
    layoutMain->addRow(tr("&IP address"), textIp = new QLineEdit);
    layoutMain->addRow(tr("&Port"), spinPort = new QSpinBox);
    spinPort->setRange(0, 65535);
    spinPort->setValue(DEFAULT_DICOM_PORT);
    layoutMain->addRow(tr("Time&out"), spinTimeout = new QSpinBox);
    spinTimeout->setValue(DEFAULT_TIMEOUT);
    spinTimeout->setSuffix(tr(" seconds"));
    spinTimeout->setRange(0, 60000);
    spinTimeout->setSingleStep(10);
    layoutMain->addItem(new QSpacerItem(30,30));

    auto layoutBtns = new QHBoxLayout;
    layoutBtns->addStretch(1);
    auto btnTest = new QPushButton(tr("&Test"));
    connect(btnTest , SIGNAL(clicked()), this, SLOT(onClickTest()));
    layoutBtns->addWidget(btnTest );
    auto btnCancel = new QPushButton(tr("Cancel"));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
    layoutBtns->addWidget(btnCancel);
    btnSave = new QPushButton(tr("Save"));
    connect(btnSave, SIGNAL(clicked()), this, SLOT(accept()));
    btnSave->setEnabled(false);
    btnSave->setDefault(true);
    layoutBtns->addWidget(btnSave);
    layoutMain->addRow(layoutBtns);

    setLayout(layoutMain);
}

void DicomServerDetails::setValues(const QString& name, const QStringList& values)
{
    textName->setText(name);
    btnSave->setEnabled(!name.isEmpty());

    if (values.count() > 0)
        textAet->setText(values.at(0));
    if (values.count() > 1)
        textIp->setText(values.at(1));
    if (values.count() > 2)
        spinPort->setValue(values.at(2).toInt());
    if (values.count() > 3)
        spinTimeout->setValue(values.at(3).toInt());
}

QString DicomServerDetails::name() const
{
    return textName->text();
}

QStringList DicomServerDetails::values() const
{
    return QStringList()
        << textAet->text()
        << textIp->text()
        << QString::number(spinPort->value())
        << QString::number(spinTimeout->value())
        ;
}

void DicomServerDetails::onClickTest()
{
    QWaitCursor wait(this);
    DcmClient client(UID_VerificationSOPClass);
    auto status = client.cEcho(textAet->text(),
       textIp->text().append(':').append(QString::number(spinPort->value())), spinTimeout->value());
    QMessageBox::information(this, windowTitle(), tr("Server check result:\n\n%1\n").arg(status));
}

void DicomServerDetails::onNameChanged()
{
    btnSave->setEnabled(!textName->text().isEmpty());
}
