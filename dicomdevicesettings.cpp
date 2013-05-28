#include "dicomdevicesettings.h"
#include <QCheckBox>
#include <QFormLayout>
#include <QLineEdit>

DicomDeviceSettings::DicomDeviceSettings(QWidget *parent) :
    QWidget(parent)
{
    QFormLayout* mainLayout = new QFormLayout;
    mainLayout->addRow(tr("&Name"), new QLineEdit);
    mainLayout->addRow(tr("AE &title"), new QLineEdit);
    mainLayout->addRow(tr("&IP address"), new QLineEdit);
    mainLayout->addRow(tr("&Port"), new QLineEdit);
    mainLayout->addRow(nullptr, new QCheckBox(tr("&Export video to DICOM")));
    setLayout(mainLayout);
}
