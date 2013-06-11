#ifndef PATIENTDIALOG_H
#define PATIENTDIALOG_H

#include <QDialog>

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
    void savePatientFile(const QString& outputPath);

protected:
    void closeEvent(QCloseEvent *evt);

signals:

public slots:
    void onShowWorklist();

};

#endif // PATIENTDIALOG_H
