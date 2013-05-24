#include "patientdialog.h"
#include <QBoxLayout>
#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QSettings>

PatientDialog::PatientDialog(QWidget *parent) :
    QDialog(parent)
{
    QFormLayout* layoutMain = new QFormLayout;
    layoutMain->addRow(tr("&Patient id"), textPatientId = new QLineEdit);
    layoutMain->addRow(tr("&Name"), textPatientName = new QLineEdit);
    layoutMain->addRow(tr("&Sex"), cbPatientSex = new QComboBox);
    QStringList sexes;
    (sexes << tr("female") << tr("male") << tr("other") << tr("unspecified")).sort();
    cbPatientSex->addItems(sexes);
    layoutMain->addRow(tr("&Birthday"), dateBirthday = new QDateEdit);
    dateBirthday->setCalendarPopup(true);
    dateBirthday->setDisplayFormat(tr("MM/dd/yyyy"));
    layoutMain->addRow(tr("Study &type"), cbStudyType = new QComboBox);
    cbStudyType->addItems(QSettings().value("studies").toStringList());

    QHBoxLayout *layoutBtns = new QHBoxLayout;
    layoutBtns->addStretch(1);
    QPushButton *btnStart = new QPushButton(tr("Start study"));
    connect(btnStart, SIGNAL(clicked()), this, SLOT(accept()));
    btnStart->setDefault(true);
    layoutBtns->addWidget(btnStart);
    layoutMain->addRow(layoutBtns);

    setLayout(layoutMain);
}

QString PatientDialog::patientName() const
{
    return textPatientName->text();
}

QString PatientDialog::studyName() const
{
    return cbStudyType->currentText();
}
