#include "dicomserverdetails.h"
#include "dcmclient.h"
#include <dcmtk/dcmdata/dcuid.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>

#ifdef QT_DEBUG
#define DEFAULT_TIMEOUT 3 // 3 seconds for test builds
#else
#define DEFAULT_TIMEOUT 30 // 30 seconds for prodaction builds
#endif

DicomServerDetails::DicomServerDetails(QWidget *parent) :
    QDialog(parent)
{
    QFormLayout* layoutMain = new QFormLayout;
    layoutMain->addRow(tr("&Name"), textName = new QLineEdit);
    connect(textName, SIGNAL(editingFinished()), this, SLOT(onNameChanged()));
    layoutMain->addRow(tr("&AE title"), textAet = new QLineEdit);
    layoutMain->addRow(tr("&IP address"), textIp = new QLineEdit);
    layoutMain->addRow(tr("&Port"), spinPort = new QSpinBox);
    spinPort->setRange(0, 65535);
    layoutMain->addRow(tr("Time&out"), spinTimeout = new QSpinBox);
    spinTimeout->setValue(DEFAULT_TIMEOUT);
    spinTimeout->setSuffix(tr(" seconds"));
    spinTimeout->setRange(0, 60000);
    spinTimeout->setSingleStep(10);
    layoutMain->addRow(nullptr, checkEcho = new QCheckBox(tr("&Echo")));
    checkEcho->setChecked(true);

    QHBoxLayout* layoutSopClass = new QHBoxLayout;
    layoutSopClass->addWidget(radioNew = new QRadioButton(tr("Ne&w")));
    radioNew->setChecked(true);
    layoutSopClass->addWidget(radioRetire = new QRadioButton(tr("&Retire")));
    layoutSopClass->addStretch(1);
    layoutMain->addRow(tr("SOP class"), layoutSopClass);
    layoutMain->addItem(new QSpacerItem(30,30));

    QHBoxLayout *layoutBtns = new QHBoxLayout;
    layoutBtns->addStretch(1);
    QPushButton *btnTest = new QPushButton(tr("&Test"));
    connect(btnTest , SIGNAL(clicked()), this, SLOT(onClickTest()));
    layoutBtns->addWidget(btnTest );
    QPushButton *btnCancel = new QPushButton(tr("Reject"));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
    layoutBtns->addWidget(btnCancel);
    btnSave = new QPushButton(tr("Save"));
    connect(btnSave, SIGNAL(clicked()), this, SLOT(accept()));
    btnSave->setEnabled(false);
    btnSave->setDefault(true);
    layoutBtns->addWidget(btnSave);
    layoutMain->addRow(layoutBtns);

    setLayout(layoutMain);
}

void DicomServerDetails::setValues(const QString& name, const QStringList& values)
{
    textName->setText(name);
    btnSave->setEnabled(!name.isEmpty());

    if (values.count() > 0)
        textAet->setText(values.at(0));
    if (values.count() > 1)
        textIp->setText(values.at(1));
    if (values.count() > 2)
        spinPort->setValue(values.at(2).toInt());
    if (values.count() > 3)
        spinTimeout->setValue(values.at(3).toInt());
    if (values.count() > 4)
        checkEcho->setChecked(values.at(4) == "Echo");
    if (values.count() > 5)
        radioNew->setChecked(values.at(5) == "New");

    radioRetire->setChecked(!radioNew->isChecked());
}

QString DicomServerDetails::name() const
{
    return textName->text();
}

QStringList DicomServerDetails::values() const
{
    return QStringList() << textAet->text() << textIp->text()
        << QString::number(spinPort->value()) << QString::number(spinTimeout->value())
        << QString(checkEcho->isChecked()? "Echo": "No echo")
        << QString(radioNew->isChecked()? "New": "Retire")
        ;
}

void DicomServerDetails::onClickTest()
{
    DcmClient client(UID_VerificationSOPClass);
    auto status = client.cEcho(textAet->text(), textIp->text().append(':').append(QString::number(spinPort->value())), spinTimeout->value());
    QMessageBox::information(this, windowTitle(), tr("Server check result:\n\n%1\n").arg(status));
}

void DicomServerDetails::onNameChanged()
{
    btnSave->setEnabled(!textName->text().isEmpty());
}
