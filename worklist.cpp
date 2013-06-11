#include "worklist.h"
#include "detailsdialog.h"
#include "dcmclient.h"

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
    activeConnection(nullptr)
{
    setWindowTitle(tr("Worklist"));

    QStringList cols = QSettings().value("worklist-columns1").toStringList();
    if (cols.size() == 0)
    {
        // Defaults are id, name, bithday, procedure description, date, time, physitian, status
        cols << "0010,0020" << "0010,0010" << "0010,0030" << "0040,0007" << "0040,0002" << "0040,0003" << "0040,0006" << "0040,0020";
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
    }
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto layoutMain = new QVBoxLayout();
    layoutMain->addWidget(createToolBar());
    layoutMain->addWidget(table);
    setLayout(layoutMain);

    setMinimumSize(640, 480);

    QSettings settings;
    restoreGeometry(settings.value("worklist-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("worklist-state").toInt());

    // Start loading of worklist right after the window is shown for the first time
    //
    QTimer::singleShot(0, this, SLOT(onLoadClick()));
    setAttribute(Qt::WA_DeleteOnClose, false);
}

Worklist::~Worklist()
{
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
    int row = table->rowCount();
    table->setRowCount(row + 1);

    for (int col = 0; col < table->columnCount(); ++col)
    {
        const char *str = nullptr;
        auto tag = table->horizontalHeaderItem(col)->data(Qt::UserRole).toInt();

        OFCondition cond = dset->findAndGetString(DcmTagKey(tag >> 16, tag & 0xFFFF), str, true);
        auto item = new QTableWidgetItem(QString::fromUtf8(str? str: cond.text()));
        table->setItem(row, col, item);
    }
    table->item(row, 0)->setData(Qt::UserRole, QVariant::fromValue(*(DcmDataset*)dset->clone()));
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

    table->setSortingEnabled(true);
    table->scrollToItem(table->currentItem());
    table->setFocus();
    table->setUpdatesEnabled(true);

    actionLoad->setEnabled(true);
    actionDetail->setEnabled(table->rowCount() > 0);
    actionStartStudy->setEnabled(table->rowCount() > 0);
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
    onShowDetailsClick();
}

void Worklist::onStartStudyClick()
{
    auto ds = getPatientDS();
    if (ds)
    {
        startStudy(ds);
    }
}

DcmDataset* Worklist::getPatientDS()
{
    int row = table->currentRow();
    return row < 0? nullptr:
        new DcmDataset(table->item(row, 0)->data(Qt::UserRole).value<DcmDataset>());
}
