#include "worklistquerysettings.h"
#include <QBoxLayout>
#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>

WorklistQuerySettings::WorklistQuerySettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    auto scheduledDate = settings.value("worklist-by-date", 1).toInt();

    QFrame *frameDate = new QFrame();
    frameDate->setFrameShape(QFrame::Box);
    frameDate->setFrameShadow(QFrame::Sunken);
    QVBoxLayout* layoutDate = new QVBoxLayout;
    layoutDate->addWidget(radioToday = new QRadioButton(tr("&Today")));
    radioToday->setChecked(scheduledDate == 1);

    QHBoxLayout* layoutToday = new QHBoxLayout;
    layoutToday->addWidget(radioTodayDelta = new QRadioButton(tr("To&day +/-")));
    radioTodayDelta->setChecked(scheduledDate == 2);
    spinDelta = new QSpinBox;
    spinDelta->setMaximum(365*10);
    spinDelta->setSuffix(tr(" day(s)"));
    spinDelta->setValue(settings.value("worklist-delta", 30).toInt());
    layoutToday->addWidget(spinDelta);
    layoutToday->addStretch(1);
    layoutDate->addLayout(layoutToday);

    QHBoxLayout* layoutRange = new QHBoxLayout;
    layoutRange->addWidget(radioRange = new QRadioButton(tr("&From")));
    radioRange->setChecked(scheduledDate == 3);
    dateFrom = new QDateEdit;
    dateFrom->setCalendarPopup(true);
    dateFrom->setDisplayFormat(tr("MM/dd/yyyy"));
    dateFrom->setDate(settings.value("worklist-from").toDate());
    layoutRange->addWidget(dateFrom);
    dateTo = new QDateEdit;
    dateTo->setCalendarPopup(true);
    dateTo->setDisplayFormat(tr("MM/dd/yyyy"));
    dateTo->setDate(settings.value("worklist-to").toDate());
    QLabel* lblTo = new QLabel(tr("t&o"));
    lblTo->setBuddy(dateTo);
    layoutRange->addWidget(lblTo);
    layoutRange->addWidget(dateTo);
    layoutRange->addStretch(1);
    layoutDate->addLayout(layoutRange);
    frameDate->setLayout(layoutDate);

    QFormLayout* layoutMain = new QFormLayout;
    layoutMain->addRow(checkScheduledDate = new QCheckBox(tr("&Scheduled\ndate")), frameDate);
    checkScheduledDate->setChecked(scheduledDate > 0);
    cbModality = new QComboBox;
    cbModality->setEditable(true);
    checkModality = new QCheckBox(tr("&Modality"));
    auto modality = settings.value("worklist-modality").toString();
    if (!modality.isEmpty())
    {
        checkModality->setChecked(true);
        cbModality->addItem(modality);
        cbModality->setCurrentIndex(0);
    }
    layoutMain->addRow(checkModality, cbModality);

    cbAeTitle = new QComboBox;
    cbAeTitle->setEditable(true);
    checkAeTitle = new QCheckBox(tr("&AE title"));
    auto aet = settings.value("worklist-aet").toString();
    if (!aet.isEmpty())
    {
        checkAeTitle->setChecked(true);
        cbAeTitle->addItem(aet);
        cbAeTitle->setCurrentIndex(0);
    }
    layoutMain->addRow(checkAeTitle, cbAeTitle);
    setLayout(layoutMain);
}

void WorklistQuerySettings::save()
{
    QSettings settings;
    auto scheduledDate = !checkScheduledDate->isChecked()? 0: radioToday->isChecked()? 1: radioTodayDelta->isChecked()?2: 3;
    settings.setValue("worklist-by-date", scheduledDate);
    settings.setValue("worklist-delta", spinDelta->value());
    settings.setValue("worklist-from", dateFrom->date());
    settings.setValue("worklist-to", dateTo->date());
    settings.setValue("worklist-modality", checkModality->isChecked()? cbModality->currentText(): nullptr);
    settings.setValue("worklist-aet", checkAeTitle->isChecked()? cbAeTitle->currentText(): nullptr);
}
