#include "physicianssettings.h"

#include <QBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>

PhysiciansSettings::PhysiciansSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    QLayout* mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(4,0,4,0);
    listStudies = new QListWidget;
    listStudies->addItems(settings.value("physicians").toStringList());
    for (int i = 0; i < listStudies->count(); ++i)
    {
        QListWidgetItem* item = listStudies->item(i);
        item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }

    mainLayout->addWidget(listStudies);
    QBoxLayout* buttonsLayout = new QVBoxLayout;
    QPushButton* btnAdd = new QPushButton(tr("&Add"));
    connect(btnAdd, SIGNAL(clicked()), this, SLOT(onAddClicked()));
    buttonsLayout->addWidget(btnAdd);
    btnEdit = new QPushButton(tr("&Edit"));
    connect(btnEdit, SIGNAL(clicked()), this, SLOT(onEditClicked()));
    buttonsLayout->addWidget(btnEdit);
    btnRemove = new QPushButton(tr("&Remove"));
    connect(btnRemove, SIGNAL(clicked()), this, SLOT(onRemoveClicked()));
    buttonsLayout->addWidget(btnRemove);
    buttonsLayout->addStretch(1);
    mainLayout->addItem(buttonsLayout);
    setLayout(mainLayout);

    btnEdit->setEnabled(listStudies->count());
    btnRemove->setEnabled(listStudies->count());
}

void PhysiciansSettings::onAddClicked()
{
    QListWidgetItem* item = new QListWidgetItem(tr("(new physician)"), listStudies);
    item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    listStudies->scrollToItem(item);
    listStudies->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
    listStudies->editItem(item);
    btnEdit->setEnabled(true);
    btnRemove->setEnabled(true);
}

void PhysiciansSettings::onEditClicked()
{
    listStudies->editItem(listStudies->currentItem());
}

void PhysiciansSettings::onRemoveClicked()
{
    delete listStudies->currentItem();
    btnEdit->setEnabled(listStudies->count());
    btnRemove->setEnabled(listStudies->count());
}

void PhysiciansSettings::save()
{
    QSettings settings;
    QStringList studies;
    for (int i = 0; i < listStudies->count(); ++i)
    {
        studies.append(listStudies->item(i)->text());
    }
    settings.setValue("physicians", studies);
}
