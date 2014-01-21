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
#include <QSettings>

HotKeySettings::HotKeySettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;

    auto layoutMain = new QFormLayout;
    layoutMain->setContentsMargins(4,0,4,0);

    layoutMain->addRow(tr("&Start study"), heStartStudy = new HotKeyEdit());
    heStartStudy->setKey(settings.value("hotkey-start", DEFAULT_HOTKEY_START).toInt());
    layoutMain->addRow(tr("&Take snapshot"), heTakeSnapshot = new HotKeyEdit());
    heTakeSnapshot->setKey(settings.value("hotkey-snapshot", DEFAULT_HOTKEY_SNAPSHOT).toInt());
    layoutMain->addRow(tr("&Record/pause"), heRepordPause = new HotKeyEdit());
    heRepordPause->setKey(settings.value("hotkey-record", DEFAULT_HOTKEY_RECORD).toInt());
    layoutMain->addRow(tr("&Archive"), heArchive = new HotKeyEdit());
    heArchive->setKey(settings.value("hotkey-archive", DEFAULT_HOTKEY_ARCHIVE).toInt());
    layoutMain->addRow(tr("S&ettings"), heSettings = new HotKeyEdit());
    heSettings->setKey(settings.value("hotkey-settings", DEFAULT_HOTKEY_SETTINGS).toInt());
#ifdef WITH_DICOM
    layoutMain->addRow(tr("&Worklist"), heWorklist = new HotKeyEdit());
    heWorklist->setKey(settings.value("hotkey-worklist", DEFAULT_HOTKEY_WORKLIST).toInt());
#endif
    setLayout(layoutMain);
}

void HotKeySettings::save()
{
    QSettings settings;

    settings.setValue("hotkey-record",   heRepordPause->key());
    settings.setValue("hotkey-snapshot", heTakeSnapshot->key());
    settings.setValue("hotkey-start",    heStartStudy->key());
    settings.setValue("hotkey-settings", heSettings->key());
    settings.setValue("hotkey-archive",  heArchive->key());
#ifdef WITH_DICOM
    settings.setValue("hotkey-worklist", heWorklist->key());
#endif
}
