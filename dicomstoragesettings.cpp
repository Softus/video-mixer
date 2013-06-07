#include "dicomstoragesettings.h"
#include <QBoxLayout>
#include <QListWidget>
#include <QSettings>

DicomStorageSettings::DicomStorageSettings(QWidget *parent) :
    QWidget(parent)
{
    QLayout* mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(4,0,4,0);
    mainLayout->addWidget(listServers = new QListWidget);

    setLayout(mainLayout);
}

QStringList DicomStorageSettings::checkedServers()
{
    QStringList listChecked;
    for (auto i = 0; i < listServers->count(); ++i)
    {
        auto item = listServers->item(i);
        if (item->checkState() == Qt::Checked)
        {
            listChecked << item->text();
        }
    }
    return listChecked;
}

void DicomStorageSettings::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);

    QSettings settings;
    QStringList listChecked = (listServers->count() == 0)? settings.value("storage-servers").toStringList(): checkedServers();

    settings.beginGroup("servers");
    listServers->clear();
    listServers->addItems(settings.allKeys());
    settings.endGroup();

    for (auto i = 0; i < listServers->count(); ++i)
    {
        auto item = listServers->item(i);
        item->setCheckState(listChecked.contains(item->text())? Qt::Checked: Qt::Unchecked);
    }

}

void DicomStorageSettings::save()
{
    QSettings().setValue("storage-servers", checkedServers());
}
