#include "dicomserverdetails.h"
#include <QBoxLayout>
#include <QCheckBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

DicomServerDetails::DicomServerDetails(QWidget *parent) :
    QDialog(parent)
{
    QFormLayout* mainLayout = new QFormLayout;
    mainLayout->addRow(tr("&Name"), new QLineEdit);
    mainLayout->addRow(tr("&AE title"), new QLineEdit);
    mainLayout->addRow(tr("&IP address"), new QLineEdit);
    mainLayout->addRow(tr("&Port"), new QLineEdit);
    mainLayout->addRow(tr("Time&out"), new QSpinBox);
    mainLayout->addRow(nullptr, new QCheckBox(tr("&Echo")));

    QHBoxLayout* layoutSopClass = new QHBoxLayout;
    layoutSopClass->addWidget(new QRadioButton(tr("Ne&w")));
    layoutSopClass->addWidget(new QRadioButton(tr("&Retire")));
    layoutSopClass->addStretch(1);
    mainLayout->addRow(tr("SOP class"), layoutSopClass);
    mainLayout->addItem(new QSpacerItem(30,30));

    QHBoxLayout *layoutBtns = new QHBoxLayout;
    layoutBtns->addStretch(1);
    QPushButton *btnTest = new QPushButton(tr("&Test"));
    connect(btnTest , SIGNAL(clicked()), this, SLOT(onClickTest()));
    layoutBtns->addWidget(btnTest );
    QPushButton *btnCancel = new QPushButton(tr("Reject"));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
    layoutBtns->addWidget(btnCancel);
    QPushButton *btnOk = new QPushButton(tr("Save"));
    connect(btnOk, SIGNAL(clicked()), this, SLOT(accept()));
    btnOk->setDefault(true);
    layoutBtns->addWidget(btnOk);
    mainLayout->addRow(layoutBtns);

    setLayout(mainLayout);
}

void DicomServerDetails::onClickTest()
{

}
