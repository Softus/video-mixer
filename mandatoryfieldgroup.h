/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
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

#ifndef MANDATORYFIELDGROUP_H
#define MANDATORYFIELDGROUP_H

#include <QObject>
#include <QRgb>

QT_BEGIN_NAMESPACE
class QPushButton;
class QWidget;
QT_END_NAMESPACE

class MandatoryFieldGroup : public QObject
{
    Q_OBJECT
    QRgb mandatoryFieldColor;
    void setMandatory(QWidget* widget, bool mandatory);

public:
    MandatoryFieldGroup(QObject *parent)
        : QObject(parent), okButton(0) {}

    void add(QWidget *widget);
    void remove(QWidget *widget);
    void setOkButton(QPushButton *button);

public slots:
    void clear();

private slots:
    void changed();

private:
    QList<QWidget *> widgets;
    QPushButton *okButton;
};

#endif
