#include "worklistcolumnsettings.h"
#include <QBoxLayout>
#include <QListView>

WorklistColumnSettings::WorklistColumnSettings(QWidget *parent) :
    QWidget(parent)
{
    QLayout* layouMain = new QVBoxLayout;
    layouMain->setContentsMargins(4,0,4,0);
    layouMain->addWidget(new QListView);
    setLayout(layouMain);
}
