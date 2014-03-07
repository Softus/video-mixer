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

class HotKeyEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

class HotKeySettings : public QWidget
{
    Q_OBJECT

    QPushButton *btnReset;
    HotKeyEdit  *editor;
    QTreeWidget *tree;
    QColor      defaultColor;

    void checkKeys(QTreeWidgetItem *top);

public:
    Q_INVOKABLE explicit HotKeySettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void save();
private slots:
    void treeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void resetClicked();
    void keyChanged(int key);
};

#endif // HOTKEYSETTINGS_H
