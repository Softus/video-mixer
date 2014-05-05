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
#include "confirmationsettings.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

ConfirmationSettings::ConfirmationSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    settings.beginGroup("confirmations");

    auto layoutMain = new QVBoxLayout;
    layoutMain->setContentsMargins(4,0,4,0);

    auto layoutCapture = new QVBoxLayout;
    auto grCapture = new QGroupBox(tr("Capture window"));
    grCapture->setLayout(layoutCapture);
    layoutMain->addWidget(grCapture);
    layoutCapture->addWidget(checkStartStudy = new QCheckBox(tr("&Start study")));
    checkStartStudy->setChecked(!settings.value("start-study").toBool());
    layoutCapture->addWidget(checkEndStudy = new QCheckBox(tr("&End study")));
    checkEndStudy->setChecked(settings.value("end-study").toInt() <= 0);

    auto layoutArchive = new QVBoxLayout;
    auto grArchive = new QGroupBox(tr("Archive window"));
    grArchive->setLayout(layoutArchive);
    layoutMain->addWidget(grArchive);
    layoutArchive->addWidget(checkStore = new QCheckBox(tr("S&end to storage")));
    checkStore->setChecked(!settings.value("archive-store").toBool());

    layoutMain->addStretch();
    setLayout(layoutMain);
}

void ConfirmationSettings::save(QSettings& settings)
{
    settings.beginGroup("confirmations");
    settings.setValue("archive-store",  !checkStore->isChecked());
    settings.setValue("end-study",       checkEndStudy->isChecked()? -1: QMessageBox::Yes);
    settings.setValue("start-study",    !checkStartStudy->isChecked());
    settings.endGroup();
}
