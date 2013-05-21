#include "dicommwlsettings.h"
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>

DicomMwlSettings::DicomMwlSettings(QWidget *parent) :
    QWidget(parent)
{
    QFormLayout* mainLayout = new QFormLayout;
    QComboBox* cbMwlFunction = new QComboBox;
    cbMwlFunction->setEditable(true);
    mainLayout->addRow(new QCheckBox(tr("MWL &Function")), cbMwlFunction);
    mainLayout->addRow(new QCheckBox(tr("&Start examination based on the worklist")));
    setLayout(mainLayout);
}
