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

#ifndef PHYSICIANSSETTINGS_H
#define PHYSICIANSSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
QT_END_NAMESPACE

class PhysiciansSettings : public QWidget
{
    Q_OBJECT
    QListWidget* listStudies;
    QPushButton* btnEdit;
    QPushButton* btnRemove;

public:
    Q_INVOKABLE explicit PhysiciansSettings(QWidget *parent = 0);

signals:

public slots:
    void onAddClicked();
    void onEditClicked();
    void onRemoveClicked();
    void save();
};

#endif // PHYSICIANSSETTINGS_H
