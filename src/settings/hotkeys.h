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

#ifndef HOTKEYSETTINGS_H
#define HOTKEYSETTINGS_H

#include <QWidget>
#include <QSettings>

QT_BEGIN_NAMESPACE
class HotKeyEdit;
class QCheckBox;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

class HotKeySettings : public QWidget
{
    Q_OBJECT

    QPushButton *btnReset;
    HotKeyEdit  *editor;
    QTreeWidget *tree;
    QCheckBox   *checkGlobal;
    QColor      defaultColor;

    void checkKeys(QTreeWidgetItem *top);

public:
    Q_INVOKABLE explicit HotKeySettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void save(QSettings& settings);
private slots:
    void treeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void resetClicked();
    void onSetGlobal(bool global);
    void keyChanged(int key);
};

#endif // HOTKEYSETTINGS_H
