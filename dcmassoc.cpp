#include "worklist.h"
#include "dcmassoc.h"

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

DcmAssoc::~DcmAssoc()
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
}

void DcmAssoc::loadCallback(void *callbackData,
    T_DIMSE_C_FindRQ* /*request*/, int /*responseCount*/,
    T_DIMSE_C_FindRSP* /*rsp*/, DcmDataset* responseIdentifiers)
{
    DcmAssoc* pThis = static_cast<DcmAssoc*>(callbackData);
    pThis->addRow(responseIdentifiers);
}

void DcmAssoc::abort()
{
    if (assoc)
    {
        qDebug() << "Aborting Association";
        ASC_abortAssociation(assoc);
    }
}

T_ASC_Parameters* DcmAssoc::initAssocParams()
{
    QSettings settings;
    DIC_NODENAME localHost;
    T_ASC_Parameters* params = nullptr;

    cond = ASC_createAssociationParameters(&params, settings.value("pdu-size", ASC_DEFAULTMAXPDU).toInt());
    if (cond.good())
    {
        QString callingAPTitle = settings.value("aet", qApp->applicationName().toUpper()).toString();
        QString calledAPTitle = settings.value("peer-aet").toString();
        ASC_setAPTitles(params, callingAPTitle.toUtf8(), calledAPTitle.toUtf8(), nullptr);

        /* Figure out the presentation addresses and copy the */
        /* corresponding values into the DcmAssoc parameters.*/
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

bool DcmAssoc::createAssociation()
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
            cond = makeOFCondition(0, 0, OF_error, "Accepted presentation context ID not found");
        }
    }

    qDebug() << cond.text();
    return false;
}

bool DcmAssoc::findSCU(DcmDataset* dset)
{
    if (!createAssociation())
    {
        return false;
    }

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
    cond = DIMSE_findUser(assoc, presId, &req, dset, loadCallback, static_cast<void*>(this),
        0 == timeout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, timeout, &rsp, &statusDetail);

    if (statusDetail != NULL)
    {
        //DCMNET_WARN("Status Detail:" << OFendl << DcmObject::PrintHelper(*statusDetail));
        delete statusDetail;
    }

    return cond.good();
}

QString DcmAssoc::nCreateRQ(DcmDataset* dset)
{
    if (!createAssociation())
    {
        return nullptr;
    }

    T_DIMSE_Message req, rsp;
    bzero((char*)&req, sizeof(req));
    bzero((char*)&rsp, sizeof(rsp));

    req.CommandField = DIMSE_N_CREATE_RQ;
    req.msg.NCreateRQ.MessageID = assoc->nextMsgID++;;
    strcpy(req.msg.NCreateRQ.AffectedSOPClassUID, abstractSyntax);
    req.msg.NCreateRQ.DataSetType = DIMSE_DATASET_PRESENT;

    cond = DIMSE_sendMessageUsingMemoryData(assoc, presId, &req, nullptr, dset, nullptr, nullptr);
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

bool DcmAssoc::nSetRQ(DcmDataset* dset, QString pendingSOPInstanceUID)
{
    if (!createAssociation())
    {
        return false;
    }

    T_DIMSE_Message req, rsp;

    bzero((char*)&req, sizeof(req));
    bzero((char*)&rsp, sizeof(rsp));

    req.CommandField = DIMSE_N_SET_RQ;
    req.msg.NSetRQ.MessageID = assoc->nextMsgID++;;
    strcpy(req.msg.NSetRQ.RequestedSOPClassUID, abstractSyntax);
    strcpy(req.msg.NSetRQ.RequestedSOPInstanceUID, pendingSOPInstanceUID.toAscii());
    req.msg.NSetRQ.DataSetType = DIMSE_DATASET_PRESENT;

    OFCondition cond = DIMSE_sendMessageUsingMemoryData(assoc, presId, &req, nullptr, dset, nullptr, nullptr);
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

