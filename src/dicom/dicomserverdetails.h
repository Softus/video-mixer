/*
 * Copyright (C) 2013-2015 Irkutsk Diagnostic Center.
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

#ifndef DICOMSERVERDETAILS_H
#define DICOMSERVERDETAILS_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
QT_END_NAMESPACE

class DicomServerDetails : public QDialog
{
    Q_OBJECT
    QLineEdit*    textName;
    QLineEdit*    textAet;
    QLineEdit*    textIp;
    QSpinBox*     spinPort;
    QSpinBox*     spinTimeout;
    QRadioButton* radioNew;
    QRadioButton* radioRetire;
    QCheckBox*    checkEcho;
    QPushButton*  btnSave;
public:
    explicit DicomServerDetails(QWidget *parent = 0);
    QString name() const;
    QStringList values() const;
    void setValues(const QString& name, const QStringList& values);

signals:
    
public slots:
    void onClickTest();
    void onNameChanged();
};

#endif // DICOMSERVERDETAILS_H
