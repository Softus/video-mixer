#include "worklist.h"
#include "dcmclient.h"

#include <dcmtk/dcmdata/dcdict.h>
#include <dcmtk/dcmdata/dcdicent.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcfilefo.h>

#include <dcmtk/dcmnet/assoc.h>
#include <dcmtk/dcmnet/dimse.h>

#include <QApplication>
#include <QDebug>
#include <QSettings>

#ifdef QT_DEBUG
#define DEFAULT_TIMEOUT 3 // 3 seconds for test builds
#else
#define DEFAULT_TIMEOUT 30 // 30 seconds for prodaction builds
#endif

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
    ds.insertEmptyElement(DCM_StudyID);
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

DcmClient::~DcmClient()
{
    if (assoc)
    {
        qDebug() << "Releasing Association";
        ASC_releaseAssociation(assoc);
        /* destroy the Association, i.e. free memory of T_ASC_Association* structure. This */
        /* call is the counterpart of ASC_requestDcmAssoc(...) which was called above. */
        ASC_destroyAssociation(&assoc);
        assoc = nullptr;
    }
    ASC_dropNetwork(&net);
}

void DcmClient::loadCallback(void *callbackData,
    T_DIMSE_C_FindRQ* /*request*/, int /*responseCount*/,
    T_DIMSE_C_FindRSP* /*rsp*/, DcmDataset* responseIdentifiers)
{
    DcmClient* pThis = static_cast<DcmClient*>(callbackData);
    pThis->addRow(responseIdentifiers);
}

void DcmClient::abort()
{
    if (assoc)
    {
        qDebug() << "Aborting Association";
        ASC_abortAssociation(assoc);
    }
}

T_ASC_Parameters* DcmClient::initAssocParams()
{
    QSettings settings;
    QString missedParameter = tr("Required settings parameter %1 is missing");

    QString calledPeerAddress = settings.value("peer").toString();
    if (calledPeerAddress.isEmpty())
    {
        cond = new OFConditionString(0, 2, OF_error, missedParameter.arg("peer").toLocal8Bit());
        return nullptr;
    }
    QString callingAPTitle = settings.value("aet", qApp->applicationName().toUpper()).toString();
    if (callingAPTitle.isEmpty())
    {
        cond = new OFConditionString(0, 3, OF_error, missedParameter.arg("aet").toLocal8Bit());
        return nullptr;
    }
    QString calledAPTitle = settings.value("peer-aet").toString();
    if (calledAPTitle.isEmpty())
    {
        cond = new OFConditionString(0, 4, OF_error, missedParameter.arg("peer-aet").toLocal8Bit());
        return nullptr;
    }

    DIC_NODENAME localHost;
    T_ASC_Parameters* params = nullptr;

    int timeout = settings.value("timeout", DEFAULT_TIMEOUT).toInt();
    cond = ASC_initializeNetwork(NET_REQUESTOR, 0, timeout, &net);

    if (cond.good())
    {
        cond = ASC_createAssociationParameters(&params, settings.value("pdu-size", ASC_DEFAULTMAXPDU).toInt());
        if (cond.good())
        {
            ASC_setAPTitles(params, callingAPTitle.toUtf8(), calledAPTitle.toUtf8(), nullptr);

            /* Figure out the presentation addresses and copy the */
            /* corresponding values into the DcmAssoc parameters.*/
            gethostname(localHost, sizeof(localHost) - 1);
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
    }

    qDebug() << cond.text();
    return nullptr;
}

bool DcmClient::createAssociation()
{
    /* create DcmAssoc, i.e. try to establish a network connection to another */
    /* DICOM application. This call creates an instance of T_ASC_DcmAssoc*. */
    T_ASC_Parameters* params = initAssocParams();
    if (params)
    {
        qDebug() << "Requesting DcmAssoc";
        cond = ASC_requestAssociation(net, params, &assoc);
        if (cond.good())
        {
            /* dump general information concerning the establishment of the network connection if required */
            qDebug() << "DcmAssoc accepted (max send PDV: " << assoc->sendPDVLength << ")";

            /* figure out which of the accepted presentation contexts should be used */
            presId = ASC_findAcceptedPresentationContextID(assoc, abstractSyntax);
            if (presId != 0)
            {
                return true;
            }
            cond = new OFConditionString(0, 1, OF_error, tr("Accepted presentation context ID not found").toLocal8Bit());
        }
    }

    qDebug() << cond.text();
    return false;
}

bool DcmClient::findSCU()
{
    if (!createAssociation())
    {
        return false;
    }

    DcmDataset ds;
    BuildCFindDataSet(ds);

    /* prepare C-FIND-RQ message */
    T_DIMSE_C_FindRQ req;
    T_DIMSE_C_FindRSP rsp;
    bzero(OFreinterpret_cast(char*, &req), sizeof(req));
    strcpy(req.AffectedSOPClassUID, abstractSyntax);
    req.DataSetType = DIMSE_DATASET_PRESENT;
    req.Priority = DIMSE_PRIORITY_LOW;
    /* complete preparation of C-FIND-RQ message */
    req.MessageID = assoc->nextMsgID++;

    /* if required, dump some more general information */
    //DCMNET_INFO("Find SCU Request Identifiers:" << OFendl << DcmObject::PrintHelper(*dset));

    /* finally conduct transmission of data */
    DcmDataset *statusDetail = nullptr;
    int timeout = QSettings().value("timeout", DEFAULT_TIMEOUT).toInt();
    cond = DIMSE_findUser(assoc, presId, &req, &ds, loadCallback, static_cast<void*>(this),
        0 == timeout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, timeout, &rsp, &statusDetail);

    if (statusDetail != NULL)
    {
        //DCMNET_WARN("Status Detail:" << OFendl << DcmObject::PrintHelper(*statusDetail));
        delete statusDetail;
    }

    return cond.good();
}

QString DcmClient::nCreateRQ(DcmDataset* patientDs)
{
    if (!createAssociation())
    {
        return nullptr;
    }

    DcmDataset nCreateDs;
    BuildNCreateDataSet(*patientDs, nCreateDs);

    patientDs->writeXML(std::cout << "------C-FIND------------" << std::endl);
    nCreateDs.writeXML(std::cout << "------N-Create------------" << std::endl);

    T_DIMSE_Message req, rsp;
    bzero((char*)&req, sizeof(req));
    bzero((char*)&rsp, sizeof(rsp));

    req.CommandField = DIMSE_N_CREATE_RQ;
    req.msg.NCreateRQ.MessageID = assoc->nextMsgID++;;
    strcpy(req.msg.NCreateRQ.AffectedSOPClassUID, abstractSyntax);
    req.msg.NCreateRQ.DataSetType = DIMSE_DATASET_PRESENT;

    cond = DIMSE_sendMessageUsingMemoryData(assoc, presId, &req, nullptr, &nCreateDs, nullptr, nullptr);
    if (cond.bad())
    {
        return nullptr;
    }

    /* receive response */
    int timeout = QSettings().value("timeout", DEFAULT_TIMEOUT).toInt();
    cond = DIMSE_receiveCommand(assoc, 0 == timeout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, timeout, &presId, &rsp, nullptr);
    if (cond.bad())
    {
        return nullptr;
    }

    if (rsp.CommandField != DIMSE_N_CREATE_RSP)
    {
        qDebug() << "DIMSE: Unexpected Response Command Field: "<< rsp.CommandField;
        return nullptr;
    }

    if (rsp.msg.NCreateRSP.MessageIDBeingRespondedTo != req.msg.NCreateRQ.MessageID)
    {
        qDebug() << "DIMSE: Unexpected Response MsgId: " << rsp.msg.NCreateRSP.MessageIDBeingRespondedTo
                 << " (expected: " << req.msg.NCreateRQ.MessageID << ")";
        return nullptr;
    }

    return QString::fromAscii(rsp.msg.NCreateRSP.AffectedSOPInstanceUID);
}

bool DcmClient::nSetRQ(const QString& pendingSOPInstanceUID)
{
    if (!createAssociation())
    {
        return false;
    }

    DcmDataset nSetDs;
    BuildNSetDataSet(nSetDs, true);

    nSetDs.writeXML(std::cout << "------N-Set------------" << std::endl);

    T_DIMSE_Message req, rsp;

    bzero((char*)&req, sizeof(req));
    bzero((char*)&rsp, sizeof(rsp));

    req.CommandField = DIMSE_N_SET_RQ;
    req.msg.NSetRQ.MessageID = assoc->nextMsgID++;;
    strcpy(req.msg.NSetRQ.RequestedSOPClassUID, abstractSyntax);
    strcpy(req.msg.NSetRQ.RequestedSOPInstanceUID, pendingSOPInstanceUID.toAscii());
    req.msg.NSetRQ.DataSetType = DIMSE_DATASET_PRESENT;

    OFCondition cond = DIMSE_sendMessageUsingMemoryData(assoc, presId, &req, nullptr, &nSetDs, nullptr, nullptr);
    if (cond.bad()) return false;

    /* receive response */
    int timeout = QSettings().value("timeout", DEFAULT_TIMEOUT).toInt();
    cond = DIMSE_receiveCommand(assoc, 0 == timeout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, timeout, &presId, &rsp, nullptr);
    if (cond.bad()) return false;

    if (rsp.CommandField != DIMSE_N_SET_RSP)
    {
        qDebug() << "DIMSE: Unexpected Response Command Field: "<< rsp.CommandField;
        return false;
    }

    if (rsp.msg.NSetRSP.MessageIDBeingRespondedTo != req.msg.NSetRQ.MessageID)
    {
        qDebug() << "DIMSE: Unexpected Response MsgId: " << rsp.msg.NSetRSP.MessageIDBeingRespondedTo
                 << " (expected: " << req.msg.NSetRQ.MessageID << ")";
        return false;
    }

    return true;
}

