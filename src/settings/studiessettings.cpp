/*
 * Copyright (C) 2013-2014 Irkutsk Diagnostic Center.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "studiessettings.h"
#include <QBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>

StudiesSettings::StudiesSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    auto mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(4,0,4,0);
    listStudies = new QListWidget;
    listStudies->addItems(settings.value("studies").toStringList());
    for (int i = 0; i < listStudies->count(); ++i)
    {
        listStudies->item(i)->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }

    mainLayout->addWidget(listStudies);
    auto buttonsLayout = new QVBoxLayout;
    auto btnAdd = new QPushButton(tr("&Add"));
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

void StudiesSettings::onAddClicked()
{
    auto item = new QListWidgetItem(tr("(new study)"), listStudies);
    item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    listStudies->scrollToItem(item);
    listStudies->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
    listStudies->editItem(item);
    btnEdit->setEnabled(true);
    btnRemove->setEnabled(true);
}

void StudiesSettings::onEditClicked()
{
    listStudies->editItem(listStudies->currentItem());
}

void StudiesSettings::onRemoveClicked()
{
    delete listStudies->currentItem();
    btnEdit->setEnabled(listStudies->count());
    btnRemove->setEnabled(listStudies->count());
}

void StudiesSettings::save()
{
    QSettings settings;
    QStringList studies;
    for (int i = 0; i < listStudies->count(); ++i)
    {
        studies.append(listStudies->item(i)->text());
    }
    settings.setValue("studies", studies);
}
