#include "dicomstoragesettings.h"
#include <QBoxLayout>
#include <QCheckBox>
#include <QListWidget>
#include <QSettings>

DicomStorageSettings::DicomStorageSettings(QWidget *parent) :
    QWidget(parent)
{
    QLayout* mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(4,0,4,0);
    mainLayout->addWidget(listServers = new QListWidget);
    mainLayout->addWidget(checkStoreVideoAsBinary = new QCheckBox(tr("Store video files as regular binary")));
    checkStoreVideoAsBinary->setChecked(QSettings().value("store-video-as-binary", false).toBool());

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
    QStringList serverNames = parent()->property("DICOM servers").toStringList();

    if (serverNames.isEmpty())
    {
        settings.beginGroup("servers");
        serverNames = settings.allKeys();
        settings.endGroup();
    }

    listServers->clear();
    listServers->addItems(serverNames);

    for (auto i = 0; i < listServers->count(); ++i)
    {
        auto item = listServers->item(i);
        item->setCheckState(listChecked.contains(item->text())? Qt::Checked: Qt::Unchecked);
    }

}

void DicomStorageSettings::save()
{
    QSettings settings;
    settings.setValue("storage-servers", checkedServers());
    settings.setValue("store-video-as-binary", checkStoreVideoAsBinary->isChecked());
}
