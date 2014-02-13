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
class DcmDataset;

class PatientDataDialog : public QDialog
{
    Q_OBJECT
    QLineEdit* textAccessionNumber;
    QLineEdit* textPatientId;
    QLineEdit* textPatientName;
    QComboBox* cbPatientSex;
    QDateEdit* dateBirthday;
    QComboBox* cbPhysician;
    QComboBox* cbStudyDescription;

public:
    explicit PatientDataDialog(bool noWorklist = false, QWidget *parent = 0);

    void readPatientFile(const QString& filePath);
    void savePatientFile(const QString& filePath);
#ifdef WITH_DICOM
    void readPatientData(DcmDataset* patient);
    void savePatientData(DcmDataset* patient);
#endif
    QString accessionNumber() const;
    QString patientId() const;
    QString patientName() const;
    QDate   patientBirthDate() const;
    QString patientBirthDateStr() const;
    QString patientSex() const;
    QChar   patientSexCode() const;
    QString studyDescription() const;
    QString physician() const;
    void setAccessionNumber(const QString& accessionNumber);
    void setPatientId(const QString& id);
    void setPatientName(const QString& name);
    void setPatientBirthDate(const QDate& date);
    void setPatientBirthDateStr(const QString& strDate);

    void setPatientSex(const QString& sex);
    void setStudyDescription(const QString& name);
    void setPhysician(const QString& name);

protected:
    virtual void showEvent(QShowEvent *);
    virtual void hideEvent(QHideEvent *);

signals:

public slots:
    void onShowWorklist();

};

#endif // PATIENTDIALOG_H
