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
    setMinimumSize(480, 200);

    auto layoutMain = new QFormLayout;
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

void PatientDialog::closeEvent(QCloseEvent *evt)
{
    QSettings settings;
    settings.setValue("new-patient-geometry", saveGeometry());
    settings.setValue("new-patient-state", (int)windowState() & ~Qt::WindowMinimized);
    QWidget::closeEvent(evt);
}

QString PatientDialog::patientName() const
{
    return textPatientName->text();
}

QString PatientDialog::studyName() const
{
    return cbStudyType->currentText();
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
