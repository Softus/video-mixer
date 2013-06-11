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
    QComboBox* cbStudyType;

public:
    explicit PatientDialog(QWidget *parent = 0);
    QString patientName() const;
    QString studyName() const;
    void setPatientId(const QString& id);
    void setPatientName(const QString& name);
    void setPatientBirthDate(const QDate& date);
    void setPatientSex(const QString& sex);
    void setStudyName(const QString& name);
    void savePatientFile(const QString& outputPath);

protected:
    virtual void done(int);

signals:

public slots:
    void onShowWorklist();

};

#endif // PATIENTDIALOG_H
