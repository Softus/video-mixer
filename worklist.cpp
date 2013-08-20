/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
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

#include "worklist.h"
#include "detailsdialog.h"
#include "dcmclient.h"
#include "transcyrillic.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include "qwaitcursor.h"

#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcitem.h>
#include <dcmtk/dcmdata/dcuid.h>

Q_DECLARE_METATYPE(DcmDataset)
static int DcmDatasetMetaType = qRegisterMetaType<DcmDataset>();

Worklist::Worklist(QWidget *parent) :
    QWidget(parent),
    timeColumn(-1),
    dateColumn(-1),
    activeConnection(nullptr)
{
    QSettings settings;
    setWindowTitle(tr("Worklist - %1").arg(settings.value("mwl-server").toString()));

    auto translateCyrillic = settings.value("translate-cyrillic", true).toBool();
    auto cols = settings.value("worklist-columns").toStringList();
    if (cols.size() == 0)
    {
        // Defaults are id, name, bithday, sex, procedure description, date, time
        cols << "0010,0020" << "0010,0010" << "0010,0030" << "0010,0040" << "0040,0007" << "0040,0002" << "0040,0003";
    }

    table = new QTableWidget(0, cols.size());
    connect(table, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(onCellDoubleClicked(QTableWidgetItem*)));

    for (auto i = 0; i < cols.size(); ++i)
    {
        DcmTag tag;
        auto text = DcmTag::findTagFromName(cols[i].toUtf8(), tag).good()? QString::fromUtf8(tag.getTagName()): cols[i];
        auto item = new QTableWidgetItem(text);
        item->setData(Qt::UserRole, (tag.getGroup() << 16) | tag.getElement());
        table->setHorizontalHeaderItem(i, item);

        if (tag == DCM_ScheduledProcedureStepStartDate)
        {
            dateColumn = i;
        }
        else if (tag == DCM_ScheduledProcedureStepStartTime)
        {
            timeColumn = i;
        }
        else if (translateCyrillic && (tag == DCM_PatientName || tag == DCM_ScheduledPerformingPhysicianName))
        {
            translateColumns.append(i);
        }
    }
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto layoutMain = new QVBoxLayout();
    layoutMain->addWidget(createToolBar());
    layoutMain->addWidget(table);
    setLayout(layoutMain);

    setMinimumSize(640, 480);

    restoreGeometry(settings.value("worklist-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("worklist-state").toInt());
    table->horizontalHeader()->restoreState(settings.value("worklist-columns-width").toByteArray());

    // Start loading of worklist right after the window is shown for the first time
    //
    QTimer::singleShot(0, this, SLOT(onLoadClick()));
    setAttribute(Qt::WA_DeleteOnClose, false);
}

QToolBar* Worklist::createToolBar()
{
    QToolBar* bar = new QToolBar(tr("Worklist"));
    bar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    actionLoad   = bar->addAction(QIcon(":/buttons/refresh"), tr("&Refresh"), this, SLOT(onLoadClick()));
    actionDetail = bar->addAction(QIcon(":/buttons/details"), tr("&Details"), this, SLOT(onShowDetailsClick()));
    actionDetail->setEnabled(false);
    actionStartStudy = bar->addAction(QIcon(":/buttons/start"), tr("Start &study"), this, SLOT(onStartStudyClick()));
    actionStartStudy->setEnabled(false);

    return bar;
}

void Worklist::onAddRow(DcmDataset* dset)
{
    QDateTime date;
    int row = table->rowCount();
    table->setRowCount(row + 1);

    for (int col = 0; col < table->columnCount(); ++col)
    {
        const char *str = nullptr;
        auto tag = table->horizontalHeaderItem(col)->data(Qt::UserRole).toInt();
        DcmTagKey tagKey(tag >> 16, tag & 0xFFFF);
        OFCondition cond = dset->findAndGetString(tagKey, str, true);
        auto text = QString::fromUtf8(str? str: cond.text());

        if (str && translateColumns.contains(col))
        {
            text = translateToCyrillic(text);
        }

        auto item = new QTableWidgetItem(text);
        table->setItem(row, col, item);
        if (col == dateColumn)
        {
            date.setDate(QDate::fromString(text, "yyyyMMdd"));
        }
        else if (col == timeColumn)
        {
            date.setTime(QTime::fromString(text, "HHmm"));
        }
    }

    table->item(row, 0)->setData(Qt::UserRole, QVariant::fromValue(*(DcmDataset*)dset->clone()));

    if (date < QDateTime::currentDateTime() && date > maxDate)
    {
       maxDate = date;
       table->selectRow(row);
    }

    qApp->processEvents();
}

void Worklist::closeEvent(QCloseEvent *evt)
{
    // Force drop connection to the server if the worklist still loading
    //
    if (activeConnection)
    {
        activeConnection->abort();
        activeConnection = nullptr;
    }
    QSettings settings;
    settings.setValue("worklist-geometry", saveGeometry());
    settings.setValue("worklist-state", (int)windowState() & ~Qt::WindowMinimized);
    settings.setValue("worklist-columns-width", table->horizontalHeader()->saveState());
    QWidget::closeEvent(evt);
}

void Worklist::onLoadClick()
{
    QWaitCursor wait(this);
    actionLoad->setEnabled(false);
    actionDetail->setEnabled(false);

    // Clear all data
    //
    table->setUpdatesEnabled(false);
    table->setSortingEnabled(false);
    maxDate.setMSecsSinceEpoch(0);
    table->setRowCount(0);

    DcmClient assoc(UID_FINDModalityWorklistInformationModel);

    activeConnection = &assoc;
    connect(&assoc, SIGNAL(addRow(DcmDataset*)), this, SLOT(onAddRow(DcmDataset*)));
    bool ok = assoc.findSCU();
    activeConnection = nullptr;

    if (!ok)
    {
        qCritical() << assoc.lastError();
        QMessageBox::critical(this, windowTitle(), assoc.lastError(), QMessageBox::Ok);
    }

    if (timeColumn >= 0)
    {
        table->sortItems(timeColumn);
        if (dateColumn >= 0)
        {
            table->sortItems(dateColumn);
        }
    }

    actionDetail->setEnabled(table->rowCount() > 0);
    actionStartStudy->setEnabled(table->rowCount() > 0);

    table->setSortingEnabled(true);
    table->scrollToItem(table->currentItem());
    table->setFocus();
    table->setUpdatesEnabled(true);

    actionLoad->setEnabled(true);
}

void Worklist::onShowDetailsClick()
{
    QWaitCursor wait(this);
    int row = table->currentRow();
    auto ds = table->item(row, 0)->data(Qt::UserRole).value<DcmDataset>();
    DetailsDialog dlg(&ds, this);
    dlg.exec();
}

void Worklist::onCellDoubleClicked(QTableWidgetItem* item)
{
    table->setCurrentItem(item);
    //onShowDetailsClick();
    onStartStudyClick();
}

void Worklist::onStartStudyClick()
{
    int row = table->currentRow();
    if (row >= 0)
    {
        auto ds = table->item(row, 0)->data(Qt::UserRole).value<DcmDataset>();
        startStudy(&ds);
    }
}
