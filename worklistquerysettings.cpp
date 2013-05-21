#include "worklistquerysettings.h"
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
#include <QCalendarWidget>

WorklistQuerySettings::WorklistQuerySettings(QWidget *parent) :
    QWidget(parent)
{
    QFrame *frameDate = new QFrame();
    frameDate->setFrameShape(QFrame::Box);
    frameDate->setFrameShadow(QFrame::Sunken);
    QVBoxLayout* layoutDate = new QVBoxLayout;
    layoutDate->addWidget(new QRadioButton(tr("&Today")));

    QHBoxLayout* layoutToday = new QHBoxLayout;
    layoutToday->addWidget(new QRadioButton(tr("To&day +/-")));
    QSpinBox* spinDelta = new QSpinBox;
    spinDelta->setMaximum(365*10);
    spinDelta->setSuffix(tr(" day(s)"));
    layoutToday->addWidget(spinDelta);
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
    frameDate->setLayout(layoutDate);

    QFormLayout* layoutMain = new QFormLayout;
    layoutMain->addRow(new QCheckBox(tr("&Scheduled date")), frameDate);
    QComboBox* cbModality = new QComboBox;
    cbModality->setEditable(true);
    layoutMain->addRow(new QCheckBox(tr("&Modality")), cbModality);
    QComboBox* cbAeTitle = new QComboBox;
    cbAeTitle->setEditable(true);
    layoutMain->addRow(new QCheckBox(tr("&AE title")), cbAeTitle);
    setLayout(layoutMain);
}
