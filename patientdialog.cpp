#include "patientdialog.h"
#include "product.h"
#include <QBoxLayout>
#include <QComboBox>
#include <QDateEdit>
#include <QDebug>
#include <QFormLayout>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QSettings>

PatientDialog::PatientDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("New patient"));
    setMinimumSize(480, 240);

    auto layoutMain = new QFormLayout;
    layoutMain->addRow(tr("&Patient id"), textPatientId = new QLineEdit);
    layoutMain->addRow(tr("&Name"), textPatientName = new QLineEdit);
    layoutMain->addRow(tr("&Sex"), cbPatientSex = new QComboBox);
    cbPatientSex->addItems(QStringList() << tr("unspecified") << tr("female") << tr("male") << tr("other"));
    layoutMain->addRow(tr("&Birthday"), dateBirthday = new QDateEdit);
    dateBirthday->setCalendarPopup(true);
    dateBirthday->setDisplayFormat(tr("MM/dd/yyyy"));

    layoutMain->addRow(tr("P&hysician"), cbPhysician = new QComboBox);
    cbPhysician->addItems(QSettings().value("physicians").toStringList());
    cbPhysician->setEditable(true);

    layoutMain->addRow(tr("Study &type"), cbStudyType = new QComboBox);
    cbStudyType->addItems(QSettings().value("studies").toStringList());
    cbStudyType->setEditable(true);

    auto layoutBtns = new QHBoxLayout;

#ifdef WITH_DICOM
    auto *btnWorklist = new QPushButton(QIcon(":buttons/show_worklist"), nullptr);
    btnWorklist->setToolTip(tr("Show work list"));
    connect(btnWorklist, SIGNAL(clicked()), this, SLOT(onShowWorklist()));
    layoutBtns->addWidget(btnWorklist);
#endif

    layoutBtns->addStretch(1);

    auto *btnReject = new QPushButton(tr("Reject"));
    connect(btnReject, SIGNAL(clicked()), this, SLOT(reject()));
    layoutBtns->addWidget(btnReject);

    auto btnStart = new QPushButton(tr("Start study"));
    connect(btnStart, SIGNAL(clicked()), this, SLOT(accept()));
    btnStart->setDefault(true);
    layoutBtns->addWidget(btnStart);

    layoutMain->addRow(layoutBtns);

    setLayout(layoutMain);
    QSettings settings;
    restoreGeometry(settings.value("new-patient-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("new-patient-state").toInt());
}

void PatientDialog::done(int result)
{
    QSettings settings;
    settings.setValue("new-patient-geometry", saveGeometry());
    settings.setValue("new-patient-state", (int)windowState() & ~Qt::WindowMinimized);
    QDialog::done(result);
}

QString PatientDialog::patientId() const
{
    return textPatientId->text();
}

QString PatientDialog::patientName() const
{
    return textPatientName->text();
}

QString PatientDialog::studyName() const
{
    return cbStudyType->currentText();
}

QString PatientDialog::physician() const
{
    return cbPhysician->currentText();
}

void PatientDialog::setPatientId(const QString& id)
{
    textPatientId->setText(id);
}

void PatientDialog::setPatientName(const QString& name)
{
    textPatientName->setText(name);
}

void PatientDialog::setPatientSex(const QString& sex)
{
    QString real;

    if (sex.length() != 1)
    {
        real = sex;
    }
    else
    {
        switch (sex[0].unicode())
        {
        case 'f':
        case 'F':
            real = tr("female");
            break;
        case 'm':
        case 'M':
            real = tr("male");
            break;
        case 'o':
        case 'O':
            real = tr("other");
            break;
        default:
            real = tr("unspecified");
            break;
        }
    }

    auto idx = cbPatientSex->findText(real);
    cbPatientSex->setCurrentIndex(idx);
    if (idx < 0)
    {
        cbPatientSex->setEditable(true);
        cbPatientSex->setEditText(real);
    }
}

void PatientDialog::setPatientBirthDate(const QDate& date)
{
    dateBirthday->setDate(date);
}

void PatientDialog::setStudyName(const QString& name)
{
    cbStudyType->setEditText(name);
}

void PatientDialog::setPhysician(const QString& physician)
{
    cbPhysician->setEditText(physician);
}

void PatientDialog::savePatientFile(const QString& outputPath)
{
    QSettings settings(outputPath, QSettings::IniFormat);

    settings.beginGroup(PRODUCT_SHORT_NAME);
    settings.setValue("patient-id", textPatientId->text());
    settings.setValue("name", textPatientName->text());
    settings.setValue("sex", cbPatientSex->currentText());
    settings.setValue("birthday", dateBirthday->text());
    settings.setValue("study", cbStudyType->currentText());
    settings.endGroup();
}

void PatientDialog::onShowWorklist()
{
    done(SHOW_WORKLIST_RESULT);
}
