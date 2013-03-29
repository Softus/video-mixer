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

Q_DECLARE_METATYPE(DcmDataset)
static int DcmDatasetMetaType = qRegisterMetaType<DcmDataset>();

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

    table = new QTableWidget(0, 7);
    table->setHorizontalHeaderLabels(QStringList() << "PatientID" << "PatientName" << "Date" << "Time" << "Physician" << "Description" << "Status");
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
        errorOF(&cond);
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

static T_ASC_Parameters*
initAssocParams(const char* abstractSyntax)
{
    QSettings settings;
    DIC_NODENAME localHost;
    T_ASC_Parameters* params = nullptr;

    OFCondition cond = ASC_createAssociationParameters(&params, settings.value("pdu-size", ASC_DEFAULTMAXPDU).toInt());
    if (cond.good())
    {
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
        cond = ASC_addPresentationContext(params, 1, abstractSyntax,
            transferSyntaxes, sizeof(transferSyntaxes)/sizeof(transferSyntaxes[0]));

        if (cond.good())
        {
            return params;
        }

        ASC_destroyAssociationParameters(&params);
    }

    qDebug() << cond.text();
    return nullptr;
}

uchar Worklist::createAssociationAndGetPresentationContextID(const char* abstractSyntax)
{
    /* create association, i.e. try to establish a network connection to another */
    /* DICOM application. This call creates an instance of T_ASC_Association*. */
    T_ASC_Parameters* params = initAssocParams(abstractSyntax);

    qDebug() << "Requesting association";
    OFCondition cond = ASC_requestAssociation(net, params, &assoc);
    if (cond.bad())
    {
        errorOF(&cond);
    }
    else if (ASC_countAcceptedPresentationContexts(params) == 0)
    {
        error(tr("No acceptable presentation contexts found"));
        ASC_releaseAssociation(assoc);
    }
    else
    {
        /* figure out which of the accepted presentation contexts should be used */
        int presId = ASC_findAcceptedPresentationContextID(assoc, abstractSyntax);
        if (presId != 0)
        {
            return presId;
        }
        error(tr("Presentation context not accepted"));
        ASC_releaseAssociation(assoc);
    }
    ASC_destroyAssociationParameters(&params);
    return 0;
}

void Worklist::errorOF(const OFCondition* cond)
{
    error(QString::fromUtf8(cond->text()));
}

static QTableWidgetItem* addItem(QTableWidget* table, DcmItem* dset, const DcmTagKey& tagKey, int row, int column)
{
    const char *str = nullptr;
    OFCondition cond = dset->findAndGetString(tagKey, str);
    auto item = new QTableWidgetItem(QString::fromUtf8(str? str: cond.text()));
    table->setItem(row, column, item);
    return item;
}

void Worklist::addRow(DcmDataset* dset)
{
    int row = table->rowCount();
    table->setRowCount(row + 1);

    addItem(table, dset, DCM_PatientID,   row, 0)->
        setData(Qt::UserRole, QVariant::fromValue(*(DcmDataset*)dset->clone()));
    addItem(table, dset, DCM_PatientName, row, 1);

    DcmItem* procedureStepSeq = nullptr;
    dset->findAndGetSequenceItem(DCM_ScheduledProcedureStepSequence, procedureStepSeq);
    if (procedureStepSeq)
    {
        QString datetime;
        datetime += addItem(table, procedureStepSeq, DCM_ScheduledProcedureStepStartDate, row, 2)->text();
        datetime += addItem(table, procedureStepSeq, DCM_ScheduledProcedureStepStartTime, row, 3)->text();
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

bool Worklist::findSCU(uchar presId, DcmDataset* dset)
{
    /* prepare C-FIND-RQ message */
    T_DIMSE_C_FindRQ req;
    T_DIMSE_C_FindRSP rsp;
    bzero(OFreinterpret_cast(char*, &req), sizeof(req));
    strcpy(req.AffectedSOPClassUID, UID_FINDModalityWorklistInformationModel);
    req.DataSetType = DIMSE_DATASET_PRESENT;
    req.Priority = DIMSE_PRIORITY_LOW;
    /* complete preparation of C-FIND-RQ message */
    req.MessageID = assoc->nextMsgID++;

    /* if required, dump some more general information */
    //DCMNET_INFO("Find SCU Request Identifiers:" << OFendl << DcmObject::PrintHelper(*dset));

    /* finally conduct transmission of data */
    DcmDataset *statusDetail = nullptr;
    int timeout = QSettings().value("timeout", DEFAULT_TIMEOUT).toInt();
    OFCondition cond = DIMSE_findUser(assoc, presId, &req, dset, loadCallback, static_cast<void*>(this),
        0 == timeout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, timeout, &rsp, &statusDetail);

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
    btnLoad->setEnabled(false);

    // Clear all data
    //
    table->setSortingEnabled(false);
    table->setRowCount(0);
    table->setUpdatesEnabled(false);
    maxDate.setMSecsSinceEpoch(0);

    QSettings settings;
    DcmDataset ds;

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
        QString modality = settings.value("modality").toString().toUpper();
        QString callingAPTitle = settings.value("aet", qApp->applicationName().toUpper()).toString();
        sps->putAndInsertString(DCM_Modality, modality.toUtf8());
        sps->putAndInsertString(DCM_ScheduledStationAETitle, callingAPTitle.toUtf8());
        sps->insertEmptyElement(DCM_ScheduledProcedureStepStartDate);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepStartTime);
        sps->insertEmptyElement(DCM_ScheduledPerformingPhysicianName);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepDescription);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepStatus);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepID);
    }

    uchar presId = createAssociationAndGetPresentationContextID(UID_FINDModalityWorklistInformationModel);
    if (presId)
    {
        /* dump general information concerning the establishment of the network connection if required */
        qDebug() << "Association accepted (max send PDV: " << assoc->sendPDVLength << ")";
        setCursor(Qt::WaitCursor);
        findSCU(presId, &ds);
        unsetCursor();
        qDebug() << "Releasing Association";
        ASC_releaseAssociation(assoc);
        /* destroy the association, i.e. free memory of T_ASC_Association* structure. This */
        /* call is the counterpart of ASC_requestAssociation(...) which was called above. */
        ASC_destroyAssociation(&assoc);
    }

    table->sortItems(3); // By time
    table->sortItems(2); // By date
    table->setSortingEnabled(true);
    table->scrollToItem(table->currentItem());
    table->setFocus();
    table->setUpdatesEnabled(true);

    btnLoad->setEnabled(true);
}

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

bool Worklist::nCreateRQ(uchar presId, DcmDataset* dset)
{
    T_DIMSE_Message req, rsp;

    bzero((char*)&req, sizeof(req));
    bzero((char*)&rsp, sizeof(rsp));

    req.CommandField = DIMSE_N_CREATE_RQ;
    req.msg.NCreateRQ.MessageID = assoc->nextMsgID++;;
    strcpy(req.msg.NCreateRQ.AffectedSOPClassUID, UID_ModalityPerformedProcedureStepSOPClass);
    req.msg.NCreateRQ.DataSetType = DIMSE_DATASET_PRESENT;

    OFCondition cond = DIMSE_sendMessageUsingMemoryData(assoc, presId, &req, nullptr, dset, nullptr, nullptr);
    if (cond.bad()) return false;

    /* receive response */
    int timeout = QSettings().value("timeout", DEFAULT_TIMEOUT).toInt();
    cond = DIMSE_receiveCommand(assoc, 0 == timeout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, timeout, &presId, &rsp, nullptr);
    if (cond.bad()) return false;

    if (rsp.CommandField != DIMSE_N_CREATE_RSP)
    {
        char buf1[256];
        sprintf(buf1, "DIMSE: Unexpected Response Command Field: 0x%x", (unsigned)rsp.CommandField);
        return false;
    }

    if (rsp.msg.NCreateRSP.MessageIDBeingRespondedTo != req.msg.NCreateRQ.MessageID)
    {
        char buf1[256];
        sprintf(buf1, "DIMSE: Unexpected Response MsgId: %d (expected: %d)", rsp.msg.NCreateRSP.MessageIDBeingRespondedTo, req.msg.NCreateRQ.MessageID);
        return false;
    }

    pendingSOPInstanceUID = rsp.msg.NCreateRSP.AffectedSOPInstanceUID;
    return true;
}

bool Worklist::nSetRQ(uchar presId, DcmDataset* dset)
{
    T_DIMSE_Message req, rsp;

    bzero((char*)&req, sizeof(req));
    bzero((char*)&rsp, sizeof(rsp));

    req.CommandField = DIMSE_N_SET_RQ;
    req.msg.NSetRQ.MessageID = assoc->nextMsgID++;;
    strcpy(req.msg.NSetRQ.RequestedSOPClassUID, UID_ModalityPerformedProcedureStepSOPClass);
    strcpy(req.msg.NSetRQ.RequestedSOPInstanceUID, pendingSOPInstanceUID.toUtf8());
    req.msg.NSetRQ.DataSetType = DIMSE_DATASET_PRESENT;

    OFCondition cond = DIMSE_sendMessageUsingMemoryData(assoc, presId, &req, nullptr, dset, nullptr, nullptr);
    if (cond.bad()) return false;

    /* receive response */
    int timeout = QSettings().value("timeout", DEFAULT_TIMEOUT).toInt();
    cond = DIMSE_receiveCommand(assoc, 0 == timeout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, timeout, &presId, &rsp, nullptr);
    if (cond.bad()) return false;

    if (rsp.CommandField != DIMSE_N_SET_RSP)
    {
        char buf1[256];
        sprintf(buf1, "DIMSE: Unexpected Response Command Field: 0x%x", (unsigned)rsp.CommandField);
        return false;
    }

    if (rsp.msg.NSetRSP.MessageIDBeingRespondedTo != req.msg.NSetRQ.MessageID)
    {
        char buf1[256];
        sprintf(buf1, "DIMSE: Unexpected Response MsgId: %d (expected: %d)", rsp.msg.NSetRSP.MessageIDBeingRespondedTo, req.msg.NSetRQ.MessageID);
        return false;
    }

    pendingSOPInstanceUID.clear();
    return true;
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

    uchar presId = createAssociationAndGetPresentationContextID(UID_FINDModalityWorklistInformationModel);
    if (presId)
    {
        /* dump general information concerning the establishment of the network connection if required */
        qDebug() << "Association accepted (max send PDV: " << assoc->sendPDVLength << ")";
        setCursor(Qt::WaitCursor);
        nCreateRQ(presId, &nCreateDs);
        unsetCursor();
        qDebug() << "Releasing Association";
        ASC_releaseAssociation(assoc);
        /* destroy the association, i.e. free memory of T_ASC_Association* structure. This */
        /* call is the counterpart of ASC_requestAssociation(...) which was called above. */
        ASC_destroyAssociation(&assoc);
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
  //      pss->putAndInsertString(DCM_ProtocolName, "SOME PROTOCOL"); // TODO
        pss->insertEmptyElement(DCM_OperatorsName);
//        pss->putAndInsertString(DCM_SeriesInstanceUID, "1.2.3.4.5.6"); // TODO
        pss->insertEmptyElement(DCM_SeriesDescription);
        pss->insertEmptyElement(DCM_RetrieveAETitle);
        pss->insertEmptyElement(DCM_ReferencedImageSequence);
        pss->insertEmptyElement(DCM_ReferencedNonImageCompositeSOPInstanceSequence);
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

    uchar presId = createAssociationAndGetPresentationContextID(UID_FINDModalityWorklistInformationModel);
    if (presId)
    {
        /* dump general information concerning the establishment of the network connection if required */
        qDebug() << "Association accepted (max send PDV: " << assoc->sendPDVLength << ")";
        setCursor(Qt::WaitCursor);
        nSetRQ(presId, &nSetDs);
        unsetCursor();
        qDebug() << "Releasing Association";
        ASC_releaseAssociation(assoc);
        /* destroy the association, i.e. free memory of T_ASC_Association* structure. This */
        /* call is the counterpart of ASC_requestAssociation(...) which was called above. */
        ASC_destroyAssociation(&assoc);
    }
}

void Worklist::onAbortClick()
{
}
