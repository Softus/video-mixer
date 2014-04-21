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

#ifndef CONFIRMATIONSETTINGS_H
#define CONFIRMATIONSETTINGS_H

#include <QWidget>
#include <QSettings>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

class ConfirmationSettings : public QWidget
{
    Q_OBJECT
    QCheckBox* checkStartStudy;
    QCheckBox* checkEndStudy;
    QCheckBox* checkDelete;
    QCheckBox* checkStore;

public:
    Q_INVOKABLE explicit ConfirmationSettings(QWidget *parent = 0);

public slots:
    void save(QSettings& settings);
};

#endif // CONFIRMATIONSETTINGS_H
