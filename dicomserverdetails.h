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
