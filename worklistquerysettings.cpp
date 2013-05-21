#include "worklistquerysettings.h"
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
#include <QCalendarWidget>

WorklistQuerySettings::WorklistQuerySettings(QWidget *parent) :
    QWidget(parent)
{
    QGroupBox *grpDate = new QGroupBox();
    grpDate->setStyleSheet("QGroupBox{ border:2px solid silver; }");
    QVBoxLayout* layoutDate = new QVBoxLayout;
    layoutDate->addWidget(new QRadioButton(tr("&Today")));

    QHBoxLayout* layoutToday = new QHBoxLayout;
    layoutToday->addWidget(new QRadioButton(tr("To&day +/-")));
    layoutToday->addWidget(new QSpinBox);
    layoutToday->addStretch(1);
    layoutDate->addLayout(layoutToday);

    QHBoxLayout* layoutRange = new QHBoxLayout;
    layoutRange->addWidget(new QRadioButton(tr("&From")));
    QDateEdit* dateFrom = new QDateEdit;
    dateFrom->setCalendarPopup(true);
    layoutRange->addWidget(dateFrom);
    QDateEdit* dateTo = new QDateEdit;
    dateTo->setCalendarPopup(true);
    QLabel* lblTo = new QLabel(tr("t&o"));
    lblTo->setBuddy(dateTo);
    layoutRange->addWidget(lblTo);
    layoutRange->addWidget(dateTo);
    layoutRange->addStretch(1);
    layoutDate->addLayout(layoutRange);
    grpDate->setLayout(layoutDate);

    QFormLayout* layoutMain = new QFormLayout;
    layoutMain->addRow(new QCheckBox(tr("&Scheduled date")), grpDate);
    QComboBox* cbModality = new QComboBox;
    cbModality->setEditable(true);
    layoutMain->addRow(new QCheckBox(tr("&Modality")), cbModality);
    QComboBox* cbAeTitle = new QComboBox;
    cbAeTitle->setEditable(true);
    layoutMain->addRow(new QCheckBox(tr("&AE title")), cbAeTitle);
    setLayout(layoutMain);
}
