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

#include "hotkeysettings.h"
#include "../defaults.h"
#include "../hotkeyedit.h"
#include "../mouseshortcut.h"

#include <QBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QSettings>
#include <QTreeWidget>
#include <QTreeWidgetItem>

static QTreeWidgetItem*
newItem(const QSettings& settings , const QString& title, const QString& settingsKey, int defaultValue)
{
    auto key = settings.value(settingsKey, defaultValue).toInt();
    auto item = new QTreeWidgetItem(QStringList()
       << title
       << MouseShortcut::toString(key)
       << settingsKey
       );
    item->setData(0, Qt::UserRole, defaultValue);
    item->setData(1, Qt::UserRole, key);
    return item;
}

HotKeySettings::HotKeySettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;

    auto layoutMain = new QVBoxLayout;
    layoutMain->setContentsMargins(4,0,4,0);

    tree = new QTreeWidget;
    tree->setColumnCount(2);
    tree->setHeaderLabels(QStringList() << tr("Action") << tr("Hotkey"));
    tree->setColumnWidth(0, 350);
    tree->setColumnWidth(1, 150);
    tree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layoutMain->addWidget(tree);
    auto itemCapture = new QTreeWidgetItem(QStringList(tr("Capture window")));
    itemCapture->setFlags(Qt::ItemIsEnabled);
    defaultColor = itemCapture->textColor(1);
    tree->addTopLevelItem(itemCapture);
    itemCapture->addChild(newItem(settings, tr("Start/stop study"),     "hotkey-start",        DEFAULT_HOTKEY_START));
    itemCapture->addChild(newItem(settings, tr("Take snapshot"),        "hotkey-snapshot",     DEFAULT_HOTKEY_SNAPSHOT));
    itemCapture->addChild(newItem(settings, tr("Start clip"),           "hotkey-record-start", DEFAULT_HOTKEY_RECORD_START));
    itemCapture->addChild(newItem(settings, tr("Clip done"),            "hotkey-record-stop",  DEFAULT_HOTKEY_RECORD_STOP));
    itemCapture->addChild(newItem(settings, tr("Show Archive window"),  "hotkey-archive",      DEFAULT_HOTKEY_ARCHIVE));
    itemCapture->addChild(newItem(settings, tr("Show Settings window"), "hotkey-settings",     DEFAULT_HOTKEY_SETTINGS));
#ifdef WITH_DICOM
    itemCapture->addChild(newItem(settings, tr("Show Worklist window"), "hotkey-worklist",     DEFAULT_HOTKEY_WORKLIST));
#endif
    auto itemArchive = new QTreeWidgetItem(QStringList(tr("Archive window")));
    itemArchive->setFlags(Qt::ItemIsEnabled);
    tree->addTopLevelItem(itemArchive);
    itemArchive->addChild(newItem(settings, tr("Delete"),    "hotkey-delete",        DEFAULT_HOTKEY_DELETE));
    itemArchive->addChild(newItem(settings, tr("Upload"),    "hotkey-upload",        DEFAULT_HOTKEY_UPLOAD));
    itemArchive->addChild(newItem(settings, tr("Edit"),      "hotkey-edit",          DEFAULT_HOTKEY_EDIT));
    itemArchive->addChild(newItem(settings, tr("Up"),        "hotkey-parent-folder", DEFAULT_HOTKEY_PARENT_FOLDER));
    itemArchive->addChild(newItem(settings, tr("File browser"),  "hotkey-browse",     DEFAULT_HOTKEY_BROWSE));

    itemArchive->addChild(newItem(settings, tr("Switch mode"),  "hotkey-next-mode",     DEFAULT_HOTKEY_NEXT_MODE));
    itemArchive->addChild(newItem(settings, tr("List mode"),    "hotkey-list-mode",     DEFAULT_HOTKEY_LIST_MODE));
    itemArchive->addChild(newItem(settings, tr("Icon mode"),    "hotkey-icon-mode",     DEFAULT_HOTKEY_ICON_MODE));
    itemArchive->addChild(newItem(settings, tr("Gallery mode"), "hotkey-gallery-mode",  DEFAULT_HOTKEY_GALLERY_MODE));

    itemArchive->addChild(newItem(settings, tr("Seek backward"), "hotkey-seek-back",  DEFAULT_HOTKEY_SEEK_BACK));
    itemArchive->addChild(newItem(settings, tr("Seek forward"),  "hotkey-seek-fwd",   DEFAULT_HOTKEY_SEEK_FWD));
    itemArchive->addChild(newItem(settings, tr("Play video"),    "hotkey-play",       DEFAULT_HOTKEY_PLAY));
    itemArchive->addChild(newItem(settings, tr("Select"),        "hotkey-select",     DEFAULT_HOTKEY_SELECT));

    //itemArchive->addChild(newItem(settings, tr("Previous file"), "hotkey-prev",     DEFAULT_HOTKEY_PREV));
    //itemArchive->addChild(newItem(settings, tr("Next file"),     "hotkey-next",     DEFAULT_HOTKEY_NEXT));

#ifdef WITH_DICOM
    auto itemWorklist = new QTreeWidgetItem(QStringList(tr("Worklist window")));
    itemWorklist->setFlags(Qt::ItemIsEnabled);
    tree->addTopLevelItem(itemWorklist);
    itemWorklist->addChild(newItem(settings, tr("Show details"),    "hotkey-show-details", DEFAULT_HOTKEY_SHOW_DETAILS));
    itemWorklist->addChild(newItem(settings, tr("Reload worklist"), "hotkey-refresh",      DEFAULT_HOTKEY_REFRESH));
#endif
    tree->expandAll();
    connect(tree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(treeItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

    auto layoutEditor = new QHBoxLayout;
    layoutMain->addLayout(layoutEditor);
    auto lbl = new QLabel(tr("&Set key"));
    layoutEditor->addWidget(lbl);
    layoutEditor->addWidget(editor = new HotKeyEdit);
    editor->setEnabled(false);
    connect(editor, SIGNAL(keyChanged(int)), this, SLOT(keyChanged(int)));
    layoutEditor->addWidget(btnReset = new QPushButton(tr("&Reset")));
    connect(btnReset, SIGNAL(clicked()), this, SLOT(resetClicked()));
    lbl->setBuddy(editor);

    setLayout(layoutMain);
    for (auto grpIdx = 0; grpIdx < tree->topLevelItemCount(); ++grpIdx)
    {
        checkKeys(tree->topLevelItem(grpIdx));
    }
}

void HotKeySettings::keyChanged(int key)
{
    auto item = tree->currentItem();
    if (item)
    {
        item->setText(1, MouseShortcut::toString(key));
        item->setData(1, Qt::UserRole, key);
    }
    checkKeys(item->parent());
}

void HotKeySettings::checkKeys(QTreeWidgetItem *top)
{
    QMap<int, QTreeWidgetItem*> keys;

    for (auto keyIdx = 0; keyIdx < top->childCount(); ++keyIdx)
    {
        auto item = top->child(keyIdx);
        auto key = item->data(1, Qt::UserRole).toInt();
        auto otherItem = keys[key];
        if (key && otherItem)
        {
            item->setTextColor(1, QColor(196, 0, 0));
            otherItem->setTextColor(1, QColor(196, 0, 0));
        }
        else
        {
            keys[key] = item;
            item->setTextColor(1, defaultColor);
        }
    }
}

void HotKeySettings::resetClicked()
{
    auto key = editor->property("default-value").toInt();
    editor->setKey(key);
    keyChanged(key);
}

void HotKeySettings::treeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (previous && editor->isEnabled())
    {
        previous->setText(1, MouseShortcut::toString(editor->key()));
    }

    if (!current || current->text(1).isEmpty())
    {
        editor->setProperty("default-value", 0);
        editor->setKey(0);
        editor->setEnabled(false);
        btnReset->setEnabled(false);
        return;
    }

    editor->setProperty("default-value", current->data(0, Qt::UserRole).toInt());
    editor->setKey(current->data(1, Qt::UserRole).toInt());
    editor->setEnabled(true);
    btnReset->setEnabled(true);
}

void HotKeySettings::save()
{
    QSettings settings;

   for (auto grpIdx = 0; grpIdx < tree->topLevelItemCount(); ++grpIdx)
   {
       auto top = tree->topLevelItem(grpIdx);
       for (auto keyIdx = 0; keyIdx < top->childCount(); ++keyIdx)
       {
           auto item = top->child(keyIdx);
           settings.setValue(item->text(2), item->data(1, Qt::UserRole));
       }
   }
}
