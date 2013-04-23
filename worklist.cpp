#include "worklist.h"
#include "detailsdialog.h"
#include "dcmclient.h"

#include <QApplication>
#include <QBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QHeaderView>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTimer>
#include "qwaitcursor.h"

#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcitem.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcuid.h>

#ifdef QT_DEBUG
#define DEFAULT_TIMEOUT 3 // 3 seconds for test builds
#else
#define DEFAULT_TIMEOUT 30 // 30 seconds for production builds
#endif

static DcmTagKey columns[] =
{
    DCM_PatientID,
    DCM_PatientName,
    DCM_PatientBirthDate,
    DCM_PatientSex,
    DCM_ScheduledProcedureStepDescription,
    DCM_ScheduledProcedureStepStartDate,
    DCM_ScheduledProcedureStepStartTime,
    DCM_ScheduledPerformingPhysicianName,
    DCM_ScheduledProcedureStepStatus,
};

Q_DECLARE_METATYPE(DcmDataset)
static int DcmDatasetMetaType = qRegisterMetaType<DcmDataset>();

Worklist::Worklist(QWidget *parent) :
    BaseWidget(parent),
    activeConnection(nullptr)
{
    const QString columnNames[] =
    {
    tr("ID"),
    tr("Name"),
    tr("Birth date"),
    tr("Sex"),
    tr("Description"),
    tr("Date"),
    tr("Time"),
    tr("Physician"),
    tr("Status"),
    };

    QSettings settings;
    iconSize = settings.value("icon-size", iconSize).toInt();

    auto buttonsLayout = new QHBoxLayout();

    btnLoad = createButton(":/buttons/load_worklist", tr("&Load"), SLOT(onLoadClick()));
    buttonsLayout->addWidget(btnLoad);

    btnDetail = createButton(":/buttons/details", tr("&Details"), SLOT(onShowDetailsClick()));
    btnDetail->setEnabled(false);
    buttonsLayout->addWidget(btnDetail);

    size_t columns = sizeof(columnNames)/sizeof(columnNames[0]);
    table = new QTableWidget(0, columns);
    for (size_t i = 0; i < columns; ++i)
    {
        auto columnHeader = new QTableWidgetItem(columnNames[i]);
        table->setHorizontalHeaderItem(i, columnHeader);
    }
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto mainLayout = new QVBoxLayout();
//    mainLayout->setMenuBar(createMenu());
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addWidget(table);

    setLayout(mainLayout);

    // Start loading of worklist right after the window is shown for the first time
    //
    QTimer::singleShot(0, this, SLOT(onLoadClick()));
    setAttribute(Qt::WA_DeleteOnClose, false);
}

Worklist::~Worklist()
{
}

static QTableWidgetItem* addItem(QTableWidget* table, DcmItem* dset, int row, int column)
{
    const char *str = nullptr;
    OFCondition cond = dset->findAndGetString(columns[column], str);
    auto item = new QTableWidgetItem(QString::fromUtf8(str? str: cond.text()));
    table->setItem(row, column, item);
    return item;
}

void Worklist::onAddRow(DcmDataset* dset)
{
    int col = 0;
    int row = table->rowCount();
    table->setRowCount(row + 1);

    addItem(table, dset, row, col++)->
        setData(Qt::UserRole, QVariant::fromValue(*(DcmDataset*)dset->clone()));
    addItem(table, dset, row, col++);
    addItem(table, dset, row, col++);
    addItem(table, dset, row, col++);

    DcmItem* procedureStepSeq = nullptr;
    dset->findAndGetSequenceItem(DCM_ScheduledProcedureStepSequence, procedureStepSeq);
    if (procedureStepSeq)
    {
        addItem(table, procedureStepSeq, row, col++);

        QString dateStr;
        dateStr += addItem(table, procedureStepSeq, row, col++)->text();
        dateStr += addItem(table, procedureStepSeq, row, col++)->text();
        QDateTime date = QDateTime::fromString(dateStr, "yyyyMMddHHmm");
        if (date < QDateTime::currentDateTime() && date > maxDate)
        {
            maxDate = date;
            table->selectRow(row);
            btnDetail->setEnabled(true);
        }
        addItem(table, procedureStepSeq, row, col++);
        addItem(table, procedureStepSeq, row, col++);
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
    BaseWidget::closeEvent(evt);
}

void Worklist::onLoadClick()
{
    QWaitCursor wait(this);
    btnLoad->setEnabled(false);
    btnDetail->setEnabled(false);

    // Clear all data
    //
    table->setSortingEnabled(false);
    table->setRowCount(0);
    table->setUpdatesEnabled(false);
    maxDate.setMSecsSinceEpoch(0);

    DcmClient assoc(UID_FINDModalityWorklistInformationModel);

    activeConnection = &assoc;
    connect(&assoc, SIGNAL(addRow(DcmDataset*)), this, SLOT(onAddRow(DcmDataset*)));
    bool ok = assoc.findSCU();
    activeConnection = nullptr;

    if (!ok)
    {
        error(assoc.lastError());
    }

    table->sortItems(6); // By time
    table->sortItems(5); // By date
    table->setSortingEnabled(true);
    table->scrollToItem(table->currentItem());
    table->resizeColumnsToContents();
    table->setFocus();
    table->setUpdatesEnabled(true);

    btnLoad->setEnabled(true);
}

void Worklist::onShowDetailsClick()
{
    QWaitCursor wait(this);
    int row = table->currentRow();
    auto ds = table->item(row, 0)->data(Qt::UserRole).value<DcmDataset>();
    DetailsDialog dlg(&ds, this);
    dlg.exec();
}

DcmDataset* Worklist::getPatientDS()
{
    int row = table->currentRow();
    return row < 0? nullptr:
        new DcmDataset(table->item(row, 0)->data(Qt::UserRole).value<DcmDataset>());
}
