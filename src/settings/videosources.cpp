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
#include "../defaults.h"
#include "videosourcedetails.h"

#include <QApplication>
#include <QBoxLayout>
#include <QTreeWidget>
#include <QMessageBox>
#include <QPushButton>

#include <QGst/ElementFactory>
#include <QGst/PropertyProbe>

static QTreeWidgetItem*
newItem(const QString& name, const QString& device, const QVariantMap& parameters, bool enabled)
{
    auto title = name.isEmpty()? device: device + " (" + name + ")";
    auto item = new QTreeWidgetItem(QStringList()
                                    << title
                                    << parameters.value("modality").toString()
                                    << parameters.value("alias").toString()
                                    );
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
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

    listSources->setColumnCount(3);
    listSources->setHeaderLabels(QStringList() << tr("Device") << tr("Modality") << tr("Alias"));
    listSources->setColumnWidth(0, 320);
    listSources->setColumnWidth(1, 80);
    connect(listSources, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(onTreeItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(listSources, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));

    mainLayout->addWidget(listSources);
    QBoxLayout* buttonsLayout = new QVBoxLayout;

    // For debug purposes
    //
    if (qApp->keyboardModifiers().testFlag(Qt::ShiftModifier))
    {
        auto btnAdd = new QPushButton(tr("&Add test\nsource"));
        connect(btnAdd, SIGNAL(clicked()), this, SLOT(onAddClicked()));
        buttonsLayout->addWidget(btnAdd);
    }

    btnDetails = new QPushButton(tr("&Edit"));
    connect(btnDetails, SIGNAL(clicked()), this, SLOT(onEditClicked()));
    buttonsLayout->addWidget(btnDetails);

    btnRemove = new QPushButton(tr("&Remove"));
    connect(btnRemove, SIGNAL(clicked()), this, SLOT(onRemoveClicked()));
    buttonsLayout->addWidget(btnRemove);

    buttonsLayout->addStretch(1);
    mainLayout->addItem(buttonsLayout);
    setLayout(mainLayout);

    settings.beginGroup("gst");
    auto cnt = settings.beginReadArray("src");
    if (cnt == 0)
    {
        settings.endArray();
        // Migration from version < 1.2
        //
        addItem(settings);
    }
    else
    {
        for (int i = 0; i < cnt; ++i)
        {
            settings.setArrayIndex(i);
            addItem(settings);
        }
        settings.endArray();
    }
    settings.endGroup();

    btnDetails->setEnabled(false);
    btnRemove->setEnabled(false);
}

void VideoSources::addItem(const QSettings& settings)
{
    QString device;
    QString friendlyName;
    bool    enabled = true;
    QVariantMap parameters;

    foreach (auto key, settings.childKeys())
    {
        if (key == "device")
        {
            device = settings.value(key).toString();
        }
        else if (key == "device-name")
        {
            friendlyName = settings.value(key).toString();
        }
        else if (key == "enabled")
        {
            enabled = settings.value(key).toBool();
        }
        else
        {
            parameters[key] = settings.value(key);
        }
    }
    auto item = newItem(friendlyName, device, parameters, enabled);
    listSources->addTopLevelItem(item);
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

    auto defaultDevice = src->property(propName).toString();

    // Look for device-name for windows and "device" for linux/macosx
    //
    QGst::PropertyProbePtr propertyProbe = src.dynamicCast<QGst::PropertyProbe>();
    if (propertyProbe && propertyProbe->propertySupportsProbe(propName))
    {
        //get a list of devices that the element supports
        auto devices = propertyProbe->probeAndGetValues(propName);
        foreach (const QGlib::Value& deviceId, devices)
        {
            // Switch to the device
            //
            src->setProperty(propName, deviceId);

            auto deviceName   = deviceId.toString();
            auto friendlyName = src->property("device-name").toString();
            auto found = false;

            foreach (auto item, listSources->findItems(deviceName, Qt::MatchStartsWith))
            {
                auto currDeviceName   = item->data(0, Qt::UserRole).toString();
                auto currFriendlyName = item->data(1, Qt::UserRole).toString();

                if (currFriendlyName == friendlyName && currDeviceName == deviceName)
                {
                    found = true;
                    break;
                }
            }

            // Check for migration from version < 1.2
            //
            if (!found && listSources->topLevelItemCount() > 0)
            {
                auto item = listSources->topLevelItem(0);
                auto currDeviceName   = item->data(0, Qt::UserRole).toString();
                auto currFriendlyName = item->data(1, Qt::UserRole).toString();

                if (currDeviceName.isEmpty())
                {
                    currDeviceName = defaultDevice;
                }

                if ((currFriendlyName.isEmpty() || currFriendlyName == friendlyName) &&
                    (currDeviceName == deviceName))
                {
                    item->setData(1, Qt::UserRole, friendlyName);
                    item->setData(0, Qt::UserRole, deviceName);
                    auto title = friendlyName.isEmpty()? deviceName: deviceName + " (" + friendlyName + ")";
                    item->setText(0, title);

                    auto alias = item->text(2);
                    if (alias.isEmpty())
                    {
                        alias = "src0";
                        item->setText(2, alias);
                    }

                    auto parameters = item->data(2, Qt::UserRole).toMap();
                    parameters["alias"] = alias;
                    parameters["device-type"] = elmName;
                    item->setData(2, Qt::UserRole, parameters);
                    found = true;
                }
            }

            if (!found)
            {
                auto alias = QString("src%1").arg(listSources->topLevelItemCount());
                QVariantMap parameters;
                parameters["alias"] = alias;
                parameters["device-type"] = elmName;
                listSources->addTopLevelItem(newItem(friendlyName, deviceName, parameters, false));
            }
        }
    }
}

void VideoSources::onTreeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    btnDetails->setEnabled(current != nullptr);
    btnRemove->setEnabled(current != nullptr);
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

void VideoSources::onRemoveClicked()
{
    auto item = listSources->currentItem();
    delete item;
}

void VideoSources::onAddClicked()
{
    QVariantMap parameters;
    parameters["alias"] = QString("src%1").arg(listSources->topLevelItemCount());
    parameters["device-type"] = "videotestsrc";
    listSources->addTopLevelItem(newItem("", "Video test source", parameters, true));
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
        auto parameters = item->data(2, Qt::UserRole).toMap();
        for (auto p = parameters.begin(); p != parameters.end(); ++p)
        {
            settings.setValue(p.key(), p.value());
        }
    }
    settings.endArray();
    settings.endGroup();
}
