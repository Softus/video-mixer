#include "worklist.h"
#include "dcmassoc.h"

#include <QApplication>
#include <QBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QHeaderView>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>

#include <dcmtk/dcmdata/dcdict.h>
#include <dcmtk/dcmdata/dcdicent.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmnet/assoc.h>

#ifdef QT_DEBUG
#define DEFAULT_TIMEOUT 3 // 3 seconds for test builds
#else
#define DEFAULT_TIMEOUT 30 // 30 seconds for prodaction builds
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
//    DCM_ScheduledPerformingPhysicianName,
//    DCM_ScheduledProcedureStepStatus,
};

Q_DECLARE_METATYPE(DcmDataset)
static int DcmDatasetMetaType = qRegisterMetaType<DcmDataset>();

static void BuildNCreateDataSet(/*const*/ DcmDataset& patientDs, DcmDataset& nCreateDs)
{
    QDateTime now = QDateTime::currentDateTime();
    QSettings settings;
    QString modality = settings.value("modality").toString().toUpper();
    QString aet = settings.value("aet", qApp->applicationName().toUpper()).toString();

    nCreateDs.putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192"); // UTF-8
    patientDs.findAndInsertCopyOfElement(DCM_PatientName, &nCreateDs);
    patientDs.findAndInsertCopyOfElement(DCM_PatientID, &nCreateDs);
    patientDs.findAndInsertCopyOfElement(DCM_PatientBirthDate, &nCreateDs);
    patientDs.findAndInsertCopyOfElement(DCM_PatientSex, &nCreateDs);
    patientDs.findAndInsertCopyOfElement(DCM_ReferencedPatientSequence, &nCreateDs);
    // MPPS ID has not logic on it. It can be anything
    // The SCU have to create it but it doesn't have to be unique and the SCP
    // should not relay on its uniqueness. Here we use a timestamp
    //
    nCreateDs.putAndInsertString(DCM_PerformedProcedureStepID, now.toString("yyyyMMddHHmmsszzz").toAscii());
    nCreateDs.putAndInsertString(DCM_PerformedStationAETitle, aet.toUtf8());

    nCreateDs.insertEmptyElement(DCM_PerformedStationName);
    nCreateDs.insertEmptyElement(DCM_PerformedLocation);
    nCreateDs.putAndInsertString(DCM_PerformedProcedureStepStartDate, now.toString("yyyyMMdd").toAscii());
    nCreateDs.putAndInsertString(DCM_PerformedProcedureStepStartTime, now.toString("HHmmss").toAscii());
    // Initial status must be 'IN PROGRESS'
    //
    nCreateDs.putAndInsertString(DCM_PerformedProcedureStepStatus, "IN PROGRESS");

    nCreateDs.insertEmptyElement(DCM_PerformedProcedureStepDescription);
    nCreateDs.insertEmptyElement(DCM_PerformedProcedureTypeDescription);
    nCreateDs.insertEmptyElement(DCM_ProcedureCodeSequence);
    nCreateDs.insertEmptyElement(DCM_PerformedProcedureStepEndDate);
    nCreateDs.insertEmptyElement(DCM_PerformedProcedureStepEndTime);

    nCreateDs.putAndInsertString(DCM_Modality, modality.toUtf8());
    nCreateDs.insertEmptyElement(DCM_StudyID);
    nCreateDs.insertEmptyElement(DCM_PerformedProtocolCodeSequence);
    nCreateDs.insertEmptyElement(DCM_PerformedSeriesSequence);

    DcmItem* ssas = nullptr;
    nCreateDs.findOrCreateSequenceItem(DCM_ScheduledStepAttributesSequence, ssas);
    if (ssas)
    {
        patientDs.findAndInsertCopyOfElement(DCM_StudyInstanceUID, ssas);
        patientDs.findAndInsertCopyOfElement(DCM_AccessionNumber, ssas);
        patientDs.findAndInsertCopyOfElement(DCM_RequestedProcedureID, ssas);
        patientDs.findAndInsertCopyOfElement(DCM_RequestedProcedureDescription, ssas);

        ssas->insertEmptyElement(DCM_ReferencedStudySequence);

        DcmItem* spss = nullptr;
        patientDs.findAndGetSequenceItem(DCM_ScheduledProcedureStepSequence, spss);
        if (spss)
        {
            spss->findAndInsertCopyOfElement(DCM_ScheduledProcedureStepID, ssas);
            spss->findAndInsertCopyOfElement(DCM_ScheduledProcedureStepDescription, ssas);
            ssas->insertEmptyElement(DCM_ScheduledProtocolCodeSequence);
        }
    }
}

static void BuildNSetDataSet(DcmDataset& nSetDs, bool completed)
{
    QDateTime now = QDateTime::currentDateTime();
    nSetDs.putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192"); // UTF-8
    nSetDs.putAndInsertString(DCM_PerformedProcedureStepStatus, completed ? "COMPLETED" : "DISCONTINUED");
    nSetDs.putAndInsertString(DCM_PerformedProcedureStepEndDate, now.toString("yyyyMMdd").toAscii());
    nSetDs.putAndInsertString(DCM_PerformedProcedureStepEndTime, now.toString("HHmmss").toAscii());

    nSetDs.insertEmptyElement(DCM_PerformedProcedureStepDescription);
    nSetDs.insertEmptyElement(DCM_PerformedProcedureTypeDescription);
    nSetDs.insertEmptyElement(DCM_ProcedureCodeSequence);
    nSetDs.insertEmptyElement(DCM_PerformedProtocolCodeSequence);

    DcmItem* pss = nullptr;
    nSetDs.findOrCreateSequenceItem(DCM_PerformedSeriesSequence, pss);
    if (pss)
    {
        pss->insertEmptyElement(DCM_PerformingPhysicianName);
//      pss->putAndInsertString(DCM_ProtocolName, "SOME PROTOCOL"); // Is it required?
        pss->insertEmptyElement(DCM_OperatorsName);
//      pss->putAndInsertString(DCM_SeriesInstanceUID, "1.2.3.4.5.6"); // Is it required?
        pss->insertEmptyElement(DCM_SeriesDescription);
        pss->insertEmptyElement(DCM_RetrieveAETitle);
        pss->insertEmptyElement(DCM_ReferencedImageSequence);
        pss->insertEmptyElement(DCM_ReferencedNonImageCompositeSOPInstanceSequence);
    }
}

static void BuildCFindDataSet(DcmDataset& ds)
{
    QSettings settings;
    QString modality = settings.value("modality").toString().toUpper();
    QString aet = settings.value("aet", qApp->applicationName().toUpper()).toString();
    bool filterByDate = settings.value("filter-by-date", true).toBool();

    ds.putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192"); // UTF-8
    ds.insertEmptyElement(DCM_AccessionNumber);
    ds.insertEmptyElement(DCM_PatientName);
    ds.insertEmptyElement(DCM_PatientID);
    ds.insertEmptyElement(DCM_PatientBirthDate);
    ds.insertEmptyElement(DCM_PatientSex);
    ds.insertEmptyElement(DCM_StudyInstanceUID);
    ds.insertEmptyElement(DCM_RequestedProcedureDescription);
    ds.insertEmptyElement(DCM_RequestedProcedureID);

    DcmItem* sps = nullptr;
    ds.findOrCreateSequenceItem(DCM_ScheduledProcedureStepSequence, sps);
    if (sps)
    {
        sps->putAndInsertString(DCM_Modality, modality.toUtf8());
        sps->putAndInsertString(DCM_ScheduledStationAETitle, aet.toUtf8());
        sps->putAndInsertString(DCM_ScheduledProcedureStepStartDate,
            filterByDate? QDate::currentDate().toString("yyyyMMdd").toAscii(): nullptr);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepStartTime);
        sps->insertEmptyElement(DCM_ScheduledPerformingPhysicianName);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepDescription);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepStatus);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepID);
    }
}


Worklist::Worklist(QWidget *parent) :
    BaseWidget(parent)
{
    auto buttonsLayout = new QHBoxLayout();
    btnLoad = new QPushButton(tr("Load"));
    btnLoad->setAutoDefault(true);
    connect(btnLoad, SIGNAL(clicked()), this, SLOT(onLoadClick()));
    buttonsLayout->addWidget(btnLoad);

    auto btnDetail = new QPushButton(tr("Details"));
    buttonsLayout->addWidget(btnDetail);

    auto btnStart = new QPushButton(tr("Start"));
    connect(btnStart, SIGNAL(clicked()), this, SLOT(onStartClick()));
    buttonsLayout->addWidget(btnStart);
    auto btnDone = new QPushButton(tr("Done"));
    connect(btnDone, SIGNAL(clicked()), this, SLOT(onDoneClick()));
    buttonsLayout->addWidget(btnDone);
    auto btnAbort = new QPushButton(tr("Abort"));
    connect(btnAbort, SIGNAL(clicked()), this, SLOT(onAbortClick()));
    buttonsLayout->addWidget(btnAbort);

    table = new QTableWidget(0, sizeof(columns)/sizeof(columns[0]));
    const DcmDataDictionary &globalDataDict = dcmDataDict.rdlock();
    for (size_t i = 0; i < sizeof(columns)/sizeof(columns[0]); ++i)
    {
        auto dicent = globalDataDict.findEntry(columns[i], nullptr);
        auto tagName = dicent == nullptr? columns[i].toString(): dicent->getTagName();
        table->setHorizontalHeaderItem(i, new QTableWidgetItem(tagName.c_str()));
    }
    dcmDataDict.unlock();
    table->horizontalHeader()->resizeMode(QHeaderView::ResizeToContents);

    auto mainLayout = new QVBoxLayout();
//    mainLayout->setMenuBar(createMenu());
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addWidget(table);

    setLayout(mainLayout);

    int timeout = QSettings().value("timeout", DEFAULT_TIMEOUT).toInt();
    OFCondition cond = ASC_initializeNetwork(NET_REQUESTOR, 0, timeout, &net);
    if (cond.bad())
    {
        error(QString::fromLocal8Bit(cond.text()));
    }
    else
    {
        btnLoad->setEnabled(true);
    }
}

Worklist::~Worklist()
{
    ASC_dropNetwork(&net);
}

static QTableWidgetItem* addItem(QTableWidget* table, DcmItem* dset, const DcmTagKey& tagKey, int row, int column)
{
    const char *str = nullptr;
    OFCondition cond = dset->findAndGetString(tagKey, str);
    auto item = new QTableWidgetItem(QString::fromUtf8(str? str: cond.text()));
    table->setItem(row, column, item);
    return item;
}

void Worklist::onAddRow(DcmDataset* dset)
{
    int col = 0;
    int row = table->rowCount();
    table->setRowCount(row + 1);

    addItem(table, dset, DCM_PatientID,   row, col++)->
        setData(Qt::UserRole, QVariant::fromValue(*(DcmDataset*)dset->clone()));
    addItem(table, dset, DCM_PatientName, row, col++);
    addItem(table, dset, DCM_PatientBirthDate, row, col++);
    addItem(table, dset, DCM_PatientSex, row, col++);

    DcmItem* procedureStepSeq = nullptr;
    dset->findAndGetSequenceItem(DCM_ScheduledProcedureStepSequence, procedureStepSeq);
    if (procedureStepSeq)
    {
        addItem(table, procedureStepSeq, DCM_ScheduledProcedureStepDescription, row, col++);

        QString dateStr;
        dateStr += addItem(table, procedureStepSeq, DCM_ScheduledProcedureStepStartDate, row, col++)->text();
        dateStr += addItem(table, procedureStepSeq, DCM_ScheduledProcedureStepStartTime, row, col++)->text();
        QDateTime date = QDateTime::fromString(dateStr, "yyyyMMddHHmm");
        if (date < QDateTime::currentDateTime() && date > maxDate)
        {
            maxDate = date;
            table->selectRow(row);
        }
        addItem(table, procedureStepSeq, DCM_ScheduledPerformingPhysicianName, row, col++);
        addItem(table, procedureStepSeq, DCM_ScheduledProcedureStepStatus, row, col++);
    }
    qApp->processEvents();
}

void Worklist::closeEvent(QCloseEvent *)
{
    // Force drop connection to the server if the worklist still loading
    //
    if (activeAssoc)
    {
        activeAssoc->abort();
        activeAssoc = nullptr;
    }
}

void Worklist::onLoadClick()
{
    btnLoad->setEnabled(false);

    // Clear all data
    //
    table->setSortingEnabled(false);
    table->setRowCount(0);
    table->setUpdatesEnabled(false);
    maxDate.setMSecsSinceEpoch(0);

    DcmDataset ds;
    DcmAssoc assoc(net, UID_FINDModalityWorklistInformationModel);
    BuildCFindDataSet(ds);

    setCursor(Qt::WaitCursor);
    activeAssoc = &assoc;
    connect(&assoc, SIGNAL(addRow(DcmDataset*)), this, SLOT(onAddRow(DcmDataset*)));
    bool ok = assoc.findSCU(&ds);
    activeAssoc = nullptr;
    unsetCursor();
    if (!ok)
    {
        error(assoc.getLastError());
    }

    table->sortItems(6); // By time
    table->sortItems(5); // By date
    table->setSortingEnabled(true);
    table->scrollToItem(table->currentItem());
    table->setFocus();
    table->setUpdatesEnabled(true);

    btnLoad->setEnabled(true);
}

void Worklist::onStartClick()
{
    int row = table->currentRow();
    if (row < 0)
    {
        return;
    }

    DcmDataset nCreateDs;
    auto patientDs = table->item(row, 0)->data(Qt::UserRole).value<DcmDataset>();
    BuildNCreateDataSet(patientDs, nCreateDs);

    patientDs.writeXML(std::cout << "------C-FIND------------" << std::endl);
    nCreateDs.writeXML(std::cout << "------N-Create------------" << std::endl);

    DcmAssoc assoc(net, UID_ModalityPerformedProcedureStepSOPClass);
    setCursor(Qt::WaitCursor);
    pendingSOPInstanceUID = assoc.nCreateRQ(&nCreateDs);
    unsetCursor();
    if (pendingSOPInstanceUID.isNull())
    {
        error(assoc.getLastError());
    }
}

void Worklist::onDoneClick()
{
    if (pendingSOPInstanceUID.isEmpty())
    {
       return;
    }

    DcmDataset nSetDs;
    BuildNSetDataSet(nSetDs, true);

    nSetDs.writeXML(std::cout << "------N-Set------------" << std::endl);

    DcmAssoc assoc(net, UID_ModalityPerformedProcedureStepSOPClass);
    setCursor(Qt::WaitCursor);
    bool ok = assoc.nSetRQ(&nSetDs, pendingSOPInstanceUID);
    unsetCursor();
    pendingSOPInstanceUID.clear();
    if (!ok)
    {
        error(assoc.getLastError());
    }
}

void Worklist::onAbortClick()
{
}
