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

#ifndef DICOMSTORAGESETTINGS_H
#define DICOMSTORAGESETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QListWidget;
QT_END_NAMESPACE

class DicomStorageSettings : public QWidget
{
    Q_OBJECT
    QListWidget* listServers;
    QCheckBox*   checkStoreVideoAsBinary;

    QStringList  checkedServers();
public:
    Q_INVOKABLE explicit DicomStorageSettings(QWidget *parent = 0);

protected:
    virtual void showEvent(QShowEvent *);

signals:
    
public slots:
    void save();
};

#endif // DICOMSTORAGESETTINGS_H
