#include "dicomserversettings.h"
#include "dicomserverdetails.h"
#include <QBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QSettings>

DicomServerSettings::DicomServerSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(4,0,4,0);
    mainLayout->addWidget(listServers = new QTableWidget);
    connect(listServers, SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)), this, SLOT(onCellChanged(QTableWidgetItem*,QTableWidgetItem*)));
    connect(listServers, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(onCellDoubleClicked(QTableWidgetItem*)));
    listServers->setColumnCount(2);
    listServers->setSelectionBehavior(QAbstractItemView::SelectRows);
    listServers->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listServers->setSortingEnabled(true);

    settings.beginGroup("servers");
    auto keys = settings.allKeys();
    listServers->setUpdatesEnabled(false);
    listServers->setRowCount(keys.count());
    for (int row = 0; row < keys.count(); ++row)
    {
        auto item = new QTableWidgetItem(keys[row]);
        auto values = settings.value(keys[row]).toStringList();
        listServers->setItem(row, 0, item);
        if (values.count() > 0)
        {
            listServers->setItem(row, 1, new QTableWidgetItem(values[0]));
        }
        item->setData(Qt::UserRole, values);
    }
    settings.endGroup();
    listServers->resizeColumnsToContents();
    listServers->setUpdatesEnabled(true);

    QBoxLayout* buttonsLayout = new QVBoxLayout;
    QPushButton* btnAdd = new QPushButton(tr("&Add"));
    connect(btnAdd, SIGNAL(clicked()), this, SLOT(onAddClicked()));
    buttonsLayout->addWidget(btnAdd);

    btnEdit = new QPushButton(tr("&Edit"));
    btnEdit->setEnabled(false);
    connect(btnEdit, SIGNAL(clicked()), this, SLOT(onEditClicked()));
    buttonsLayout->addWidget(btnEdit);

    btnRemove = new QPushButton(tr("&Remove"));
    btnRemove->setEnabled(false);
    connect(btnRemove, SIGNAL(clicked()), this, SLOT(onRemoveClicked()));
    buttonsLayout->addWidget(btnRemove);
    buttonsLayout->addStretch(1);

    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);
}

void DicomServerSettings::onCellChanged(QTableWidgetItem* current, QTableWidgetItem*)
{
    btnEdit->setEnabled(current != nullptr);
    btnRemove->setEnabled(current != nullptr);
}

void DicomServerSettings::onCellDoubleClicked(QTableWidgetItem*)
{
    onEditClicked();
}

void DicomServerSettings::onAddClicked()
{
    DicomServerDetails dlg(this);
    if (dlg.exec())
    {
        auto row = listServers->rowCount();
        listServers->setRowCount(row + 1);

        auto item = new QTableWidgetItem(dlg.aet());
        listServers->setItem(row, 0, item);
        listServers->setItem(row, 1, new QTableWidgetItem(dlg.aet()));
        item->setData(Qt::UserRole, dlg.values());
    }
}

void DicomServerSettings::onEditClicked()
{
    auto item = listServers->item(listServers->currentRow(), 0);
    DicomServerDetails dlg(this);
    dlg.setValues(item->text(), item->data(Qt::UserRole).toStringList());
    if (dlg.exec())
    {
        auto values = dlg.values();
        item->setData(Qt::UserRole, values);
        item->setText(dlg.aet());
        if (values.count() > 0)
        {
            listServers->item(item->row(), 1)->setText(values[0]);
        }
    }
}

void DicomServerSettings::onRemoveClicked()
{
    listServers->removeRow(listServers->currentRow());
    btnEdit->setEnabled(listServers->rowCount());
    btnRemove->setEnabled(listServers->rowCount());
}

void DicomServerSettings::save()
{
    QSettings settings;

    settings.remove("servers");
    settings.beginGroup("servers");
    for (int i = 0; i < listServers->rowCount(); ++i)
    {
        auto item = listServers->item(i, 0);
        settings.setValue(item->text(), item->data(Qt::UserRole));
    }
    settings.endGroup();
}
