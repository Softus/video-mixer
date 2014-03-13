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

#ifndef WORKLISTQUERYSETTINGS_H
#define WORKLISTQUERYSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDateEdit;
class QRadioButton;
class QSpinBox;
QT_END_NAMESPACE

class WorklistQuerySettings : public QWidget
{
    Q_OBJECT
    QCheckBox*    checkScheduledDate;
    QRadioButton* radioToday;
    QRadioButton* radioTodayDelta;
    QRadioButton* radioRange;
    QSpinBox*     spinDelta;
    QDateEdit*    dateFrom;
    QDateEdit*    dateTo;
    QCheckBox*    checkModality;
    QComboBox*    cbModality;
    QCheckBox*    checkAeTitle;
    QComboBox*    cbAeTitle;

public:
    Q_INVOKABLE explicit WorklistQuerySettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void save();
};

#endif // WORKLISTQUERYSETTINGS_H
