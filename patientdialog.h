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

#ifndef PATIENTDIALOG_H
#define PATIENTDIALOG_H

#include <QDialog>
#include <QDate>

#define SHOW_WORKLIST_RESULT 100

QT_BEGIN_NAMESPACE
class QComboBox;
class QDateEdit;
class QLineEdit;
QT_END_NAMESPACE

class PatientDialog : public QDialog
{
    Q_OBJECT
    QLineEdit* textPatientId;
    QLineEdit* textPatientName;
    QComboBox* cbPatientSex;
    QDateEdit* dateBirthday;
    QComboBox* cbPhysician;
    QComboBox* cbStudyType;

public:
    explicit PatientDialog(QWidget *parent = 0);
    QString patientId() const;
    QString patientName() const;
    QDate   patientBirthDate() const;
    QString patientBirthDateStr() const;
    QString patientSex() const;
    QChar   patientSexCode() const;
    QString studyName() const;
    QString physician() const;
    void setPatientId(const QString& id);
    void setPatientName(const QString& name);
    void setPatientBirthDate(const QDate& date);
    void setPatientBirthDateStr(const QString& strDate);

    void setPatientSex(const QString& sex);
    void setStudyName(const QString& name);
    void setPhysician(const QString& name);
    void savePatientFile(const QString& outputPath);
    void setEditable(bool editable);

protected:
    virtual void done(int);

signals:

public slots:
    void onShowWorklist();

};

#endif // PATIENTDIALOG_H
