#include "dicomstoragesettings.h"
#include <QBoxLayout>
#include <QListView>

DicomStorageSettings::DicomStorageSettings(QWidget *parent) :
    QWidget(parent)
{
    QLayout* mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(4,0,4,0);
    mainLayout->addWidget(new QListView);
    setLayout(mainLayout);
}
