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

#ifndef DICOMSERVERSETTINGS_H
#define DICOMSERVERSETTINGS_H

#include <QWidget>
#include <QSettings>

QT_BEGIN_NAMESPACE
class QTableWidgetItem;
class QTableWidget;
class QPushButton;
QT_END_NAMESPACE

class DicomServerSettings : public QWidget
{
    Q_OBJECT
    QTableWidget* listServers;
    QPushButton* btnEdit;
    QPushButton* btnRemove;

public:
    Q_INVOKABLE explicit DicomServerSettings(QWidget *parent = 0);
    void updateColumns(int row, const QStringList& values);
    void updateServerList();

signals:

public slots:
    void save(QSettings& settings);

private slots:
    void onCellChanged(QTableWidgetItem* current, QTableWidgetItem* previous);
    void onCellDoubleClicked(QTableWidgetItem* item);
    void onAddClicked();
    void onEditClicked();
    void onRemoveClicked();
};

#endif // DICOMSERVERSETTINGS_H
