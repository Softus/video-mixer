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

#include "videosources.h"
#include "videosourcedetails.h"

#include <QApplication>
#include <QBoxLayout>
#include <QTreeWidget>
#include <QMessageBox>
#include <QPushButton>

#include <QGst/ElementFactory>
#include <QGst/PropertyProbe>

static QTreeWidgetItem*
newItem(const QString& name, const QString& device, const QVariantMap& parameters, bool enabled = true)
{
    auto title = name.isEmpty()? device: device + " (" + name + ")";
    auto item = new QTreeWidgetItem(QStringList()
                                    << title
                                    << parameters.value("modality").toString()
                                    << parameters.value("alias").toString()
                                    );
    item->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
    item->setCheckState(0, enabled? Qt::Checked: Qt::Unchecked);
    item->setData(0, Qt::UserRole, device);
    item->setData(1, Qt::UserRole, name);
    item->setData(2, Qt::UserRole, parameters);
    return item;
}

VideoSources::VideoSources(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    QLayout* mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(4,0,4,0);
    listSources = new QTreeWidget;

    listSources->setColumnCount(2);
    listSources->setHeaderLabels(QStringList() << tr("Device") << tr("Modality") << tr("Alias"));
    listSources->setColumnWidth(0, 320);
    listSources->setColumnWidth(1, 80);
    connect(listSources, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(onTreeItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(listSources, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));

    mainLayout->addWidget(listSources);
    QBoxLayout* buttonsLayout = new QVBoxLayout;
    btnDetails = new QPushButton(tr("&Edit"));
    connect(btnDetails, SIGNAL(clicked()), this, SLOT(onEditClicked()));
    buttonsLayout->addWidget(btnDetails);
    if (qApp->keyboardModifiers().testFlag(Qt::ShiftModifier))
    {
        auto btnAdd = new QPushButton(tr("&Add test source"));
        connect(btnAdd, SIGNAL(clicked()), this, SLOT(onAddClicked()));
        buttonsLayout->addWidget(btnAdd);
    }
    buttonsLayout->addStretch(1);
    mainLayout->addItem(buttonsLayout);
    setLayout(mainLayout);

    settings.beginGroup("gst");
    auto cnt = settings.beginReadArray("src");
    for (int i = 0; i < cnt; ++i)
    {
        settings.setArrayIndex(i);
        auto device       = settings.value("device").toString();
        auto friendlyName = settings.value("device-name").toString();
        auto enabled      = settings.value("enabled").toBool();
        auto parameters   = settings.value("parameters").toMap();
        parameters["device-type"] = settings.value("device-type", PLATFORM_SPECIFIC_SOURCE);
        auto item = newItem(friendlyName, device, parameters, enabled);
        listSources->addTopLevelItem(item);
    }
    settings.endArray();
    settings.endGroup();

    btnDetails->setEnabled(false);
}

void VideoSources::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);

    // Populate cameras list
    //
    updateDeviceList(PLATFORM_SPECIFIC_SOURCE, PLATFORM_SPECIFIC_PROPERTY);

#ifndef Q_OS_WIN
    // Populate firewire list
    //
    updateDeviceList("dv1394src", "guid");
#endif
}

void VideoSources::updateDeviceList(const char* elmName, const char* propName)
{
    auto src = QGst::ElementFactory::make(elmName);
    if (!src) {
        QMessageBox::critical(this, windowTitle(), tr("Failed to create element '%1'").arg(elmName));
        return;
    }

    // Look for device-name for windows and "device" for linux/macosx
    //
    QGst::PropertyProbePtr propertyProbe = src.dynamicCast<QGst::PropertyProbe>();
    if (propertyProbe && propertyProbe->propertySupportsProbe(propName))
    {
        //get a list of devices that the element supports
        auto devices = propertyProbe->probeAndGetValues(propName);
        foreach (const QGlib::Value& device, devices)
        {
            auto deviceName = device.toString();
            auto friendlyName = src->property("device-name").toString();
            auto found = false;

            foreach (auto item, listSources->findItems(deviceName, Qt::MatchStartsWith))
            {
                if (item->data(0, Qt::UserRole).toString() == deviceName &&
                    item->data(1, Qt::UserRole).toString() == friendlyName)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                auto alias = QString("src%1").arg(listSources->topLevelItemCount());
                QVariantMap parameters;
                parameters["alias"] = alias;
                parameters["device-type"] = elmName;
                listSources->addTopLevelItem(newItem(friendlyName, deviceName, parameters));
            }
        }
    }
}

void VideoSources::onTreeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    btnDetails->setEnabled(current != nullptr);
}

void VideoSources::onItemDoubleClicked(QTreeWidgetItem*, int)
{
    onEditClicked();
}

void VideoSources::onEditClicked()
{
    auto item = listSources->currentItem();
    auto device     = item->data(0, Qt::UserRole).toString();
    auto parameters = item->data(2, Qt::UserRole).toMap();
    auto deviceType = parameters["device-type"].toString();

    VideoSourceDetails dlg(parameters, this);
    dlg.setWindowTitle(item->text(0));
    dlg.updateDevice(device, deviceType);

    if (dlg.exec())
    {
        dlg.updateParameters(parameters);
        item->setData(2, Qt::UserRole, parameters);
        item->setText(1, parameters["modality"].toString());
        item->setText(2, parameters["alias"].toString());
    }
}

void VideoSources::onAddClicked()
{
    QVariantMap parameters;
    parameters["alias"] = QString("src%1").arg(listSources->topLevelItemCount());
    parameters["device-type"] = "videotestsrc";
    listSources->addTopLevelItem(newItem("", "Video test source", parameters));
}

void VideoSources::save(QSettings& settings)
{
    settings.beginGroup("gst");
    settings.beginWriteArray("src");

    for (int i = 0; i < listSources->topLevelItemCount(); ++i)
    {
        settings.setArrayIndex(i);
        auto item = listSources->topLevelItem(i);
        settings.setValue("device", item->data(0, Qt::UserRole));
        settings.setValue("device-name", item->data(1, Qt::UserRole));
        settings.setValue("enabled", item->checkState(0) == Qt::Checked);
        settings.setValue("parameters", item->data(2, Qt::UserRole));
    }
    settings.endArray();
    settings.endGroup();
}
