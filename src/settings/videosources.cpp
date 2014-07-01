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
newItem(const QString& title, const QString& elmName, const QString& propName)
{
    auto item = new QTreeWidgetItem(QStringList() << title);
    item->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
    item->setCheckState(0, Qt::Checked);
    item->setData(0, Qt::UserRole, elmName);
    item->setData(1, Qt::UserRole, propName);
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
        auto btnAdd = new QPushButton(tr("&Add"));
        connect(btnAdd, SIGNAL(clicked()), this, SLOT(onAddClicked()));
        buttonsLayout->addWidget(btnAdd);
    }
    buttonsLayout->addStretch(1);
    mainLayout->addItem(buttonsLayout);
    setLayout(mainLayout);

    btnDetails->setEnabled(false);
}

void VideoSources::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);

    listSources->clear();

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
    QSettings settings;
    settings.beginGroup("gst");

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
            friendlyName = friendlyName.isEmpty()? deviceName: deviceName + " (" + friendlyName + ")";

            listSources->addTopLevelItem(newItem(deviceName, elmName, propName));
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
    VideoSourceDetails dlg(item->text(0), item->data(0, Qt::UserRole).toString(), item->data(1, Qt::UserRole).toString(), this);
    if (dlg.exec())
    {

    }
}

void VideoSources::onAddClicked()
{
    listSources->addTopLevelItem(newItem("videotest", "videotestsrc", "pattern"));
}

void VideoSources::save(QSettings& settings)
{
}
