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

#include "defaults.h"
#include "hotkeysettings.h"
#include "hotkeyedit.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QSettings>
#include <QVBoxLayout>

HotKeySettings::HotKeySettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;

    auto layoutMain = new QVBoxLayout;
    layoutMain->setContentsMargins(4,0,4,0);

    auto layoutCapture = new QFormLayout;
    auto grCapture = new QGroupBox(tr("Capture window"));
    grCapture->setLayout(layoutCapture);
    layoutMain->addWidget(grCapture);
    layoutCapture->addRow(tr("&Start/stop study"), heStartStudy = new HotKeyEdit());
    heStartStudy->setKey(settings.value("hotkey-start", DEFAULT_HOTKEY_START).toInt());
    layoutCapture->addRow(tr("&Take snapshot"), heTakeSnapshot = new HotKeyEdit());
    heTakeSnapshot->setKey(settings.value("hotkey-snapshot", DEFAULT_HOTKEY_SNAPSHOT).toInt());
    layoutCapture->addRow(tr("Start &clip"), heRecordStart = new HotKeyEdit());
    heRecordStart->setKey(settings.value("hotkey-record-start", DEFAULT_HOTKEY_RECORD_START).toInt());
    layoutCapture->addRow(tr("Clip d&one"), heRecordStop = new HotKeyEdit());
    heRecordStop->setKey(settings.value("hotkey-record-stop", DEFAULT_HOTKEY_RECORD_STOP).toInt());
    layoutCapture->addRow(tr("Show &Archive"), heArchive = new HotKeyEdit());
    heArchive->setKey(settings.value("hotkey-archive", DEFAULT_HOTKEY_ARCHIVE).toInt());
    layoutCapture->addRow(tr("Show S&ettings"), heSettings = new HotKeyEdit());
    heSettings->setKey(settings.value("hotkey-settings", DEFAULT_HOTKEY_SETTINGS).toInt());

#ifdef WITH_DICOM
    layoutCapture->addRow(tr("Show &Worklist"), heWorklist = new HotKeyEdit());
    heWorklist->setKey(settings.value("hotkey-worklist", DEFAULT_HOTKEY_WORKLIST).toInt());

    auto layoutWorklist = new QFormLayout;
    auto grWorklist = new QGroupBox(tr("Worklist window"));
    grWorklist->setLayout(layoutWorklist);
    layoutMain->addWidget(grWorklist);
    layoutWorklist->addRow(tr("Show &details"), heDetails = new HotKeyEdit());
    heDetails->setKey(settings.value("hotkey-show-details", DEFAULT_HOTKEY_SHOW_DETAILS).toInt());
    layoutWorklist->addRow(tr("Re&load worklist"), heRefresh = new HotKeyEdit());
    heRefresh->setKey(settings.value("hotkey-refresh", DEFAULT_HOTKEY_REFRESH).toInt());
#endif

    layoutMain->addStretch();
    setLayout(layoutMain);
}

void HotKeySettings::save()
{
    QSettings settings;

    settings.setValue("hotkey-record-start", heRecordStart->key());
    settings.setValue("hotkey-record-stop",  heRecordStop->key());
    settings.setValue("hotkey-snapshot", heTakeSnapshot->key());
    settings.setValue("hotkey-start",    heStartStudy->key());
    settings.setValue("hotkey-settings", heSettings->key());
    settings.setValue("hotkey-archive",  heArchive->key());
#ifdef WITH_DICOM
    settings.setValue("hotkey-worklist",     heWorklist->key());
    settings.setValue("hotkey-show-details", heDetails->key());
    settings.setValue("hotkey-refresh",      heRefresh->key());
#endif
}
