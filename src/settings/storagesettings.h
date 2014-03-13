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

#ifndef STORAGESETTINGS_H
#define STORAGESETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QxtLineEdit;
class QSpinBox;
QT_END_NAMESPACE

class StorageSettings : public QWidget
{
    Q_OBJECT
    QLineEdit   *textOutputPath;
    QxtLineEdit *textVideoOutputPath;
    QLineEdit   *textFolderTemplate;
    QxtLineEdit *textVideoFolderTemplate;
    QLineEdit   *textImageTemplate;
    QLineEdit   *textClipTemplate;
    QLineEdit   *textVideoTemplate;
    QCheckBox   *checkMaxVideoSize;
    QSpinBox    *spinMaxVideoSize;

public:
    Q_INVOKABLE explicit StorageSettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void save();

private slots:
    void onClickBrowse();
    void onClickVideoBrowse();
};

#endif // STORAGESETTINGS_H
