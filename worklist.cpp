#include "worklist.h"

#include <QApplication>
#include <QSettings>
#include <QDebug>
#include <QBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>

#include <dcmtk/config/cfunix.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofcond.h>    /* for class OFCondition */

#include <dcmtk/dcmnet/assoc.h>
#include <dcmtk/dcmnet/dimse.h>
//#include <dcmtk/dcmnet/diutil.h>

#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcfilefo.h>

#ifdef QT_DEBUG
#define DEFAULT_TIMEOUT 3 // 3 seconds for test builds
#else
#define DEFAULT_TIMEOUT 30 // 30 seconds for prodaction builds
#endif

Worklist::Worklist(QWidget *parent) :
    BaseWidget(parent)
{
    auto buttonsLayout = new QHBoxLayout();
    auto btnLoad = new QPushButton(tr("Load"));
    btnLoad->setAutoDefault(true);
    connect(btnLoad, SIGNAL(clicked()), this, SLOT(onLoadClick()));
    buttonsLayout->addWidget(btnLoad);

    auto btnDetail = new QPushButton(tr("Details"));
    buttonsLayout->addWidget(btnDetail);

    table = new QTableWidget(0, 7);
    table->setHorizontalHeaderLabels(QStringList() << "PatientID" << "PatientName" << "Date" << "Time" << "Physician" << "Description" << "Status");
    table->horizontalHeader()->resizeMode(QHeaderView::ResizeToContents);

    auto mainLayout = new QVBoxLayout();
//    mainLayout->setMenuBar(createMenu());
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addWidget(table);

    setLayout(mainLayout);

    initAssoc();
}

Worklist::~Worklist()
{
    termAssoc();
}

bool Worklist::initAssoc()
{
    QSettings settings;
    DIC_NODENAME localHost;

    int timeout = settings.value("timeout", DEFAULT_TIMEOUT).toInt();
    OFCondition cond = ASC_initializeNetwork(NET_REQUESTOR, 0, timeout, &net);
    if (cond.good())
    {
        cond = ASC_createAssociationParameters(&params, settings.value("pdu-size", ASC_DEFAULTMAXPDU).toInt());
        if (cond.good())
        {
            /* sets this application's title and the called application's title in the params */
            /* structure. The default values to be set here are "STORESCU" and "ANY-SCP". */
            QString callingAPTitle = settings.value("aet", qApp->applicationName().toUpper()).toString();
            QString calledAPTitle = settings.value("peer-aet").toString();
            ASC_setAPTitles(params, callingAPTitle.toUtf8(), calledAPTitle.toUtf8(), nullptr);

            /* Figure out the presentation addresses and copy the */
            /* corresponding values into the association parameters.*/
            gethostname(localHost, sizeof(localHost) - 1);
            QString calledPeerAddress = settings.value("peer").toString();
            ASC_setPresentationAddresses(params, localHost, calledPeerAddress.toUtf8());

            /* Set the presentation contexts which will be negotiated */
            /* when the network connection will be established */
#if __BYTE_ORDER == __LITTLE_ENDIAN
            const char* transferSyntaxes[] = { UID_LittleEndianExplicitTransferSyntax,
               UID_BigEndianExplicitTransferSyntax, UID_LittleEndianImplicitTransferSyntax };
#elif __BYTE_ORDER == __BIG_ENDIAN
            const char* transferSyntaxes[] = { UID_BigEndianExplicitTransferSyntax,
               UID_LittleEndianExplicitTransferSyntax, UID_LittleEndianImplicitTransferSyntax };
#else
    #error "Unsupported byte order"
#endif
            cond = ASC_addPresentationContext(params, 1, UID_FINDModalityWorklistInformationModel,
                transferSyntaxes, sizeof(transferSyntaxes)/sizeof(transferSyntaxes[0]));

            if (cond.good())
            {
                return true;
            }

            ASC_destroyAssociationParameters(&params);
        }

        ASC_dropNetwork(&net);
    }

    qDebug() << cond.text();
    return false;
}

void Worklist::termAssoc()
{
    ASC_destroyAssociationParameters(&params);
    ASC_dropNetwork(&net);
}

void Worklist::errorOF(const OFCondition* cond)
{
    error(QString::fromUtf8(cond->text()));
}

static QString addItem(QTableWidget* table, DcmItem* dset, const DcmTagKey& tagKey, int row, int column)
{
    const char *str = nullptr;
    OFCondition cond = dset->findAndGetString(tagKey, str);
    QString qstr = QString::fromUtf8(str? str: cond.text());
    table->setItem(row, column, new QTableWidgetItem(qstr));
    return qstr;
}

void Worklist::addRow(DcmDataset* dset)
{
    int row = table->rowCount();
    table->setRowCount(row + 1);

    addItem(table, dset, DCM_PatientID,   row, 0);
    addItem(table, dset, DCM_PatientName, row, 1);

    DcmItem* procedureStepSeq = nullptr;
    dset->findAndGetSequenceItem(DCM_ScheduledProcedureStepSequence, procedureStepSeq);
    if (procedureStepSeq)
    {
        QString datetime;
        datetime += addItem(table, procedureStepSeq, DCM_ScheduledProcedureStepStartDate, row, 2);
        datetime += addItem(table, procedureStepSeq, DCM_ScheduledProcedureStepStartTime, row, 3);
        QDateTime date = QDateTime::fromString(datetime, "yyyyMMddHHmm");
        if (date < QDateTime::currentDateTime() && date > maxDate)
        {
            maxDate = date;
            table->selectRow(row);
        }
        addItem(table, procedureStepSeq, DCM_ScheduledPerformingPhysicianName, row, 4);
        addItem(table, procedureStepSeq, DCM_ScheduledProcedureStepDescription, row, 5);
        addItem(table, procedureStepSeq, DCM_ScheduledProcedureStepStatus, row, 6);
    }
}

void Worklist::loadCallback(void *callbackData,
    T_DIMSE_C_FindRQ *request, int responseCount,
    T_DIMSE_C_FindRSP *rsp, DcmDataset *responseIdentifiers)
{
    /* dump response number */
    qDebug() << "Find Response: " << responseCount << " (" << rsp->DimseStatus << ", " << request->DataSetType << ")";

    /* dump data set which was received */
    //DCMNET_WARN(DcmObject::PrintHelper(*responseIdentifiers));

    static_cast<Worklist*>(callbackData)->addRow(responseIdentifiers);
    qApp->processEvents();
}

void Worklist::closeEvent(QCloseEvent *)
{
    // Force drop connection to the server if the worklist still loading
    //
    if (assoc)
    {
        ASC_abortAssociation(assoc);
    }
}

bool Worklist::findSCU(DcmDataset* dset)
{
    T_DIMSE_C_FindRQ req;
    T_DIMSE_C_FindRSP rsp;
    T_ASC_PresentationContextID presId;
    DcmDataset *statusDetail = nullptr;

    /* figure out which of the accepted presentation contexts should be used */
    presId = ASC_findAcceptedPresentationContextID(assoc, UID_FINDModalityWorklistInformationModel);

    if (presId == 0)
    {
        error(tr("Presentation context not accepted"));
        return false;
    }

    /* prepare C-FIND-RQ message */
    bzero(OFreinterpret_cast(char*, &req), sizeof(req));
    strcpy(req.AffectedSOPClassUID, UID_FINDModalityWorklistInformationModel);
    req.DataSetType = DIMSE_DATASET_PRESENT;
    req.Priority = DIMSE_PRIORITY_LOW;
    /* complete preparation of C-FIND-RQ message */
    req.MessageID = assoc->nextMsgID++;

    /* if required, dump some more general information */
    //DCMNET_INFO("Find SCU Request Identifiers:" << OFendl << DcmObject::PrintHelper(*dset));

    /* finally conduct transmission of data */
    int timeout = QSettings().value("timeout", DEFAULT_TIMEOUT).toInt();
    OFCondition cond = DIMSE_findUser(assoc, presId, &req, dset, loadCallback, static_cast<void*>(this),
        0 == timeout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, timeout, &rsp, &statusDetail);

    /* dump status detail information if there is some */
    if (statusDetail != NULL)
    {
        //DCMNET_WARN("Status Detail:" << OFendl << DcmObject::PrintHelper(*statusDetail));
        delete statusDetail;
    }

    if (cond.bad())
    {
        errorOF(&cond);
        return false;
    }

    return true;
}

void Worklist::onLoadClick()
{
    // Clear all data
    //
    table->setSortingEnabled(false);
    table->setRowCount(0);
    table->setUpdatesEnabled(false);
    maxDate.setMSecsSinceEpoch(0);

    QSettings settings;
    DcmDataset ds;
    DcmItem* procedureStepSeq = nullptr;

    ds.putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192"); // UTF-8
    ds.insertEmptyElement(DCM_AccessionNumber);
    ds.insertEmptyElement(DCM_PatientName);
    ds.insertEmptyElement(DCM_PatientID);
    ds.insertEmptyElement(DCM_PatientBirthDate);
    ds.insertEmptyElement(DCM_PatientSex);
    ds.insertEmptyElement(DCM_StudyInstanceUID);
    ds.findOrCreateSequenceItem(DCM_ScheduledProcedureStepSequence, procedureStepSeq);
    if (procedureStepSeq)
    {
        QString modality = settings.value("modality").toString().toUpper();
        QString callingAPTitle = settings.value("aet", qApp->applicationName().toUpper()).toString();
        procedureStepSeq->putAndInsertString(DCM_Modality, modality.toUtf8());
        procedureStepSeq->putAndInsertString(DCM_ScheduledStationAETitle, callingAPTitle.toUtf8());
        procedureStepSeq->insertEmptyElement(DCM_ScheduledProcedureStepStartDate);
        procedureStepSeq->insertEmptyElement(DCM_ScheduledProcedureStepStartTime);
        procedureStepSeq->insertEmptyElement(DCM_ScheduledPerformingPhysicianName);
        procedureStepSeq->insertEmptyElement(DCM_ScheduledProcedureStepDescription);
        procedureStepSeq->insertEmptyElement(DCM_ScheduledProcedureStepStatus);
    }

    /* create association, i.e. try to establish a network connection to another */
    /* DICOM application. This call creates an instance of T_ASC_Association*. */
    qDebug() << "Requesting association";
    OFCondition cond = ASC_requestAssociation(net, params, &assoc);
    if (cond.bad())
    {
        errorOF(&cond);
        return;
    }

    /* count the presentation contexts which have been accepted by the SCP */
    /* If there are none, finish the execution */
    if (ASC_countAcceptedPresentationContexts(params) == 0)
    {
        error(tr("No acceptable presentation contexts found"));
    }
    else
    {
        /* dump general information concerning the establishment of the network connection if required */
        qDebug() << "Association accepted (max send PDV: " << assoc->sendPDVLength << ")";
        setCursor(Qt::WaitCursor);
        findSCU(&ds);
        unsetCursor();
    }

    qDebug() << "Releasing Association";
    ASC_releaseAssociation(assoc);

    // Detach parameters, we still need them
    //
    assoc->params = nullptr;

    /* destroy the association, i.e. free memory of T_ASC_Association* structure. This */
    /* call is the counterpart of ASC_requestAssociation(...) which was called above. */
    ASC_destroyAssociation(&assoc);

    table->sortItems(3); // By time
    table->sortItems(2); // By date
    table->setSortingEnabled(true);
    table->scrollToItem(table->currentItem());
    table->setFocus();
    table->setUpdatesEnabled(true);
}
