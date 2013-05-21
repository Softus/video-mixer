#include "studiessettings.h"
#include <QBoxLayout>
#include <QListView>
#include <QPushButton>

StudiesSettings::StudiesSettings(QWidget *parent) :
    QWidget(parent)
{
    QLayout* mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(4,0,4,0);
    mainLayout->addWidget(new QListView);
    QBoxLayout* buttonsLayout = new QVBoxLayout;
    buttonsLayout->addWidget(new QPushButton(tr("&Add")));
    buttonsLayout->addWidget(new QPushButton(tr("&Edit")));
    buttonsLayout->addWidget(new QPushButton(tr("&Remove")));
    buttonsLayout->addStretch(1);

    mainLayout->addItem(buttonsLayout);
    setLayout(mainLayout);
}
