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
    QDate patientBirthDate() const;
    QString patientSex() const;
    QChar patientSexCode() const;
    QString studyName() const;
    QString physician() const;
    void setPatientId(const QString& id);
    void setPatientName(const QString& name);
    void setPatientBirthDate(const QDate& date);
    void setPatientSex(const QString& sex);
    void setStudyName(const QString& name);
    void setPhysician(const QString& physician);
    void savePatientFile(const QString& outputPath);
    void setEditable(bool editable);

protected:
    virtual void done(int);

signals:

public slots:
    void onShowWorklist();

};

#endif // PATIENTDIALOG_H
