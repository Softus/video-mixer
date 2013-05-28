#include "dicommppssettings.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>

DicomMppsSettings::DicomMppsSettings(QWidget *parent) :
    QWidget(parent)
{
    QFormLayout* mainLayout = new QFormLayout;
    QComboBox* cbMppsFunction = new QComboBox;
    cbMppsFunction->setEditable(true);
    mainLayout->addRow(new QCheckBox(tr("MPPS &Function")), cbMppsFunction);
    mainLayout->addRow(new QCheckBox(tr("&In progress is automatically sent when an examitaion has been started")));
    mainLayout->addRow(new QCheckBox(tr("&Completed is automatically sent when an examitaion has been ended")));
    setLayout(mainLayout);
}
