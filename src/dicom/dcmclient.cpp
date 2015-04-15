/*
 * Copyright (C) 2013-2015 Irkutsk Diagnostic Center.
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

#include "../defaults.h"
#include "../product.h"
#include "../typedetect.h"
#include "dcmclient.h"
#include "dcmconverter.h"
#include "transcyrillic.h"
#include "qxtglobal.h" // for _countof

#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcdicent.h>
#include <dcmtk/dcmdata/dcdict.h>
#include <dcmtk/dcmdata/dcfilefo.h>

#include <dcmtk/dcmnet/assoc.h>
#include <dcmtk/dcmnet/dimse.h>
#include <dcmtk/dcmnet/diutil.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

static void CopyPatientData(/*const*/ DcmDataset* src, DcmDataset* dst)
{
    src->findAndInsertCopyOfElement(DCM_AccessionNumber, dst);
    src->findAndInsertCopyOfElement(DCM_PatientName, dst);
    src->findAndInsertCopyOfElement(DCM_PatientID, dst);
    src->findAndInsertCopyOfElement(DCM_PatientBirthDate, dst);
    src->findAndInsertCopyOfElement(DCM_PatientSex, dst);
}

static void BuildCFindDataSet(DcmDataset& ds)
{
    QSettings settings;
    settings.beginGroup("dicom");

    QString modality = settings.value("worklist-modality").toString().toUpper();
    if (modality.isEmpty())
    {
        modality = settings.value("modality", DEFAULT_MODALITY).toString().toUpper();
    }

    QString aet = settings.value("worklist-aet").toString();
    if (aet.isEmpty())
    {
        aet = settings.value("aet").toString();
        if (aet.isEmpty())
            aet = qApp->applicationName().toUpper();
    }
    auto scheduledDate = settings.value("worklist-by-date", DEFAULT_WORKLIST_DATE_RANGE).toInt();

    ds.putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192"); // UTF-8
    ds.insertEmptyElement(DCM_AccessionNumber);
    ds.insertEmptyElement(DCM_PatientName);
    ds.insertEmptyElement(DCM_PatientID);
    ds.insertEmptyElement(DCM_PatientBirthDate);
    ds.insertEmptyElement(DCM_PatientSex);
    ds.insertEmptyElement(DCM_PatientSize),
    ds.insertEmptyElement(DCM_PatientWeight),
    ds.insertEmptyElement(DCM_StudyInstanceUID);
    ds.insertEmptyElement(DCM_SeriesInstanceUID);
    ds.insertEmptyElement(DCM_StudyID);
    ds.insertEmptyElement(DCM_RequestedProcedureDescription);
    ds.insertEmptyElement(DCM_RequestedProcedureID);

    DcmItem* sps = nullptr;
    ds.findOrCreateSequenceItem(DCM_ScheduledProcedureStepSequence, sps);
    if (sps)
    {
        sps->putAndInsertString(DCM_Modality, modality.toUtf8());
        sps->putAndInsertString(DCM_ScheduledStationAETitle, aet.toUtf8());
        switch (scheduledDate)
        {
        case 0: // Everything
            sps->insertEmptyElement(DCM_ScheduledProcedureStepStartDate);
            break;
        case 1: // Today
            sps->putAndInsertString(DCM_ScheduledProcedureStepStartDate,
                QDate::currentDate().toString("yyyyMMdd").toUtf8());
            break;
        case 2: // Today +- days
            {
                auto deltaDays = settings.value("worklist-delta", DEFAULT_WORKLIST_DAY_DELTA).toInt();
                auto range = QDate::currentDate().addDays(-deltaDays).toString("yyyyMMdd") + "-" +
                    QDate::currentDate().addDays(+deltaDays).toString("yyyyMMdd");
                sps->putAndInsertString(DCM_ScheduledProcedureStepStartDate, range.toUtf8());
            }
            break;
        case 3: // From .. to
            {
                auto range = settings.value("worklist-from").toDate().toString("yyyyMMdd") + "-" +
                    settings.value("worklist-to").toDate().toString("yyyyMMdd");
                sps->putAndInsertString(DCM_ScheduledProcedureStepStartDate, range.toUtf8());
            }
            break;
        }

        sps->insertEmptyElement(DCM_ScheduledProcedureStepStartTime);
        sps->insertEmptyElement(DCM_ScheduledPerformingPhysicianName);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepDescription);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepStatus);
        sps->insertEmptyElement(DCM_ScheduledProcedureStepID);
    }
}

static void BuildCStoreDataSet(/*const*/ DcmDataset& patientDs, DcmDataset& cStoreDs, const QString& seriesUID)
{
    auto now = QDateTime::currentDateTime();

    if (patientDs.findAndInsertCopyOfElement(DCM_SpecificCharacterSet, &cStoreDs).bad())
    {
        cStoreDs.putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192"); // UTF-8
    }

    CopyPatientData(&patientDs, &cStoreDs);
    patientDs.findAndInsertCopyOfElement(DCM_PerformingPhysicianName, &cStoreDs);
    patientDs.findAndInsertCopyOfElement(DCM_StudyDescription, &cStoreDs);
    patientDs.findAndInsertCopyOfElement(DCM_SeriesDescription, &cStoreDs);

    patientDs.findAndInsertCopyOfElement(DCM_StudyInstanceUID, &cStoreDs);
    cStoreDs.putAndInsertString(DCM_SeriesInstanceUID, seriesUID.toUtf8());

    cStoreDs.putAndInsertString(DCM_StudyDate, now.toString("yyyyMMdd").toUtf8());
    cStoreDs.putAndInsertString(DCM_StudyTime, now.toString("HHmmss").toUtf8());

    cStoreDs.putAndInsertString(DCM_Manufacturer, ORGANIZATION_FULL_NAME);
    cStoreDs.putAndInsertString(DCM_ManufacturerModelName, PRODUCT_FULL_NAME);
}

static void BuildNCreateDataSet(/*const*/ DcmDataset& patientDs, DcmDataset& nCreateDs)
{
    QDateTime now = QDateTime::currentDateTime();
    QSettings settings;
    settings.beginGroup("dicom");

    auto modality = settings.value("modality", DEFAULT_MODALITY).toString().toUpper().toUtf8();
    QString aet = settings.value("aet", qApp->applicationName().toUpper()).toString();

    nCreateDs.putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192"); // UTF-8
    CopyPatientData(&patientDs, &nCreateDs);
    patientDs.findAndInsertCopyOfElement(DCM_ReferencedPatientSequence, &nCreateDs);
    // MPPS ID has not logic on it. It can be anything
    // The SCU have to create it but it doesn't have to be unique and the SCP
    // should not relay on its uniqueness. Here we use a timestamp
    //
    nCreateDs.putAndInsertString(DCM_PerformedProcedureStepID, now.toString("yyyyMMddHHmmsszzz").toUtf8());
    nCreateDs.putAndInsertString(DCM_PerformedStationAETitle, aet.toUtf8());

    nCreateDs.insertEmptyElement(DCM_PerformedStationName);
    nCreateDs.insertEmptyElement(DCM_PerformedLocation);
    nCreateDs.putAndInsertString(DCM_PerformedProcedureStepStartDate, now.toString("yyyyMMdd").toUtf8());
    nCreateDs.putAndInsertString(DCM_PerformedProcedureStepStartTime, now.toString("HHmmss").toUtf8());
    // Initial status must be 'IN PROGRESS'
    //
    nCreateDs.putAndInsertString(DCM_PerformedProcedureStepStatus, "IN PROGRESS");

    nCreateDs.insertEmptyElement(DCM_PerformedProcedureStepDescription);
    nCreateDs.insertEmptyElement(DCM_PerformedProcedureTypeDescription);
    nCreateDs.insertEmptyElement(DCM_ProcedureCodeSequence);
    nCreateDs.insertEmptyElement(DCM_PerformedProcedureStepEndDate);
    nCreateDs.insertEmptyElement(DCM_PerformedProcedureStepEndTime);

    nCreateDs.putAndInsertString(DCM_Modality, modality);
    patientDs.findAndInsertCopyOfElement(DCM_StudyID, &nCreateDs);
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

static QString BuildNSetDataSet(/*const*/ DcmDataset& patientDs, DcmDataset& nSetDs, const char* seriesUID, bool completed)
{
    QDateTime now = QDateTime::currentDateTime();
    nSetDs.putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192"); // UTF-8
    CopyPatientData(&patientDs, &nSetDs);

    nSetDs.putAndInsertString(DCM_PerformedProcedureStepStatus, completed ? "COMPLETED" : "DISCONTINUED");
    nSetDs.putAndInsertString(DCM_PerformedProcedureStepEndDate, now.toString("yyyyMMdd").toUtf8());
    nSetDs.putAndInsertString(DCM_PerformedProcedureStepEndTime, now.toString("HHmmss").toUtf8());

    nSetDs.insertEmptyElement(DCM_PerformedProcedureStepDescription);
    nSetDs.insertEmptyElement(DCM_PerformedProcedureTypeDescription);
    nSetDs.insertEmptyElement(DCM_ProcedureCodeSequence);
    nSetDs.insertEmptyElement(DCM_PerformedProtocolCodeSequence);

    DcmItem* pss = nullptr;
    nSetDs.findOrCreateSequenceItem(DCM_PerformedSeriesSequence, pss);
    if (pss)
    {
        patientDs.findAndInsertCopyOfElement(DCM_PerformingPhysicianName, pss);
        patientDs.findAndInsertCopyOfElement(DCM_StudyInstanceUID, pss);

        pss->insertEmptyElement(DCM_OperatorsName);
        pss->putAndInsertString(DCM_SeriesInstanceUID, seriesUID);
        pss->insertEmptyElement(DCM_SeriesDescription);
        pss->insertEmptyElement(DCM_RetrieveAETitle);
        pss->insertEmptyElement(DCM_ReferencedImageSequence);
        pss->insertEmptyElement(DCM_ReferencedNonImageCompositeSOPInstanceSequence);
    }

    return QString::fromUtf8(seriesUID);
}

DcmClient::~DcmClient()
{
    if (assoc)
    {
        qDebug() << "Releasing Association";
        ASC_releaseAssociation(assoc);
        ASC_destroyAssociation(&assoc);
        assoc = nullptr;
    }
    ASC_dropNetwork(&net);
}

int DcmClient::timeout() const
{
    return QSettings().value("dicom/timeout", DEFAULT_TIMEOUT).toInt();
}

void DcmClient::loadCallback(void *callbackData,
    T_DIMSE_C_FindRQ* /*request*/, int /*responseCount*/,
    T_DIMSE_C_FindRSP* /*rsp*/, DcmDataset* dset)
{
    auto translateCyrillic = QSettings().value("dicom/translate-cyrillic", DEFAULT_TRANSLATE_CYRILLIC).toBool();
    if (translateCyrillic)
    {
        DcmTagKey keys[] = { DCM_PatientName, DCM_ScheduledPerformingPhysicianName };
        for (size_t i = 0; i < _countof(keys); ++i)
        {
            DcmElement* elm = nullptr;
            char* val = nullptr;
            if (dset->findAndGetElement(keys[i], elm, true).good() && elm->getString(val).good())
            {
                auto str = QString::fromUtf8(val);
                auto tranlated = translateToCyrillic(str);
                //qDebug() << str << " => " << tranlated;
                elm->putString(tranlated.toUtf8().constData());
            }
        }
    }

    static_cast<DcmClient*>(callbackData)->addRow(dset);
}

void DcmClient::abort()
{
    if (assoc)
    {
        qDebug() << "Aborting Association";
        ASC_abortAssociation(assoc);
    }
}

T_ASC_Parameters* DcmClient::initAssocParams(const QString& server, const char* transferSyntax)
{
    auto srvName = QString("servers/").append(server);
    QSettings settings;
    QString missedParameter = tr("Required settings parameter %1 is missing");

    // Format is server=AE_TITLE,host,port,timeout
    //
    auto values = settings.value(srvName).toStringList();
    if (values.isEmpty())
    {
        cond = makeOFCondition(0, 2, OF_error, missedParameter.arg(srvName).toLocal8Bit());
        return nullptr;
    }

    auto peerAet = values.takeFirst();

    auto peerAddress = values.isEmpty()? peerAet.toLower(): values.takeFirst();
    peerAddress.append(':').append(values.isEmpty()? QString::number(DEFAULT_DICOM_PORT): values.takeFirst());

    auto timeout = values.isEmpty()? DEFAULT_TIMEOUT: values.takeFirst().toInt();

    return initAssocParams(peerAet.toUtf8(), peerAddress, timeout, transferSyntax);
}

T_ASC_Parameters* DcmClient::initAssocParams(const QString& peerAet, const QString& peerAddress, int timeout, const char* transferSyntax)
{
    QSettings settings;
    settings.beginGroup("dicom");

    DIC_NODENAME localHost;
    T_ASC_Parameters* params = nullptr;

    cond = ASC_initializeNetwork(NET_REQUESTOR, settings.value("local-port").toInt(), timeout, &net);
    if (cond.good())
    {
        cond = ASC_createAssociationParameters(&params, settings.value("pdu-size", ASC_DEFAULTMAXPDU).toInt());
        if (cond.good())
        {
            ASC_setAPTitles(params, settings.value("aet", qApp->applicationName().toUpper()).toString().toUtf8(), peerAet.toUtf8(), nullptr);

            /* Figure out the presentation addresses and copy the */
            /* corresponding values into the DcmAssoc parameters.*/
            gethostname(localHost, sizeof(localHost) - 1);
            ASC_setPresentationAddresses(params, localHost, peerAddress.toUtf8());

            if (transferSyntax)
            {
                const char* arr[] = { transferSyntax };
                cond = ASC_addPresentationContext(params, 1, abstractSyntax, arr, 1);
            }
            else
            {
                /* Set the presentation contexts which will be negotiated */
                /* when the network connection will be established */
                const char* transferSyntaxes[] =
                {
#if __BYTE_ORDER == __LITTLE_ENDIAN
                    UID_LittleEndianExplicitTransferSyntax, UID_BigEndianExplicitTransferSyntax,
#elif __BYTE_ORDER == __BIG_ENDIAN
                    UID_BigEndianExplicitTransferSyntax, UID_LittleEndianExplicitTransferSyntax,
#else
#error "Unsupported byte order"
#endif
                    UID_LittleEndianImplicitTransferSyntax
                };

                cond = ASC_addPresentationContext(params, 1, abstractSyntax,
                    transferSyntaxes, sizeof(transferSyntaxes)/sizeof(transferSyntaxes[0]));
            }

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

bool DcmClient::createAssociation(const QString& server, const char* transferSyntax)
{
    if (assoc)
    {
        // Already got a server association
        //
        return true;
    }

    // Create DcmAssoc, i.e. try to establish a network connection to another
    // DICOM application. This call creates an instance of T_ASC_DcmAssoc*.
    //
    T_ASC_Parameters* params = initAssocParams(server, transferSyntax);
    if (params)
    {
        qDebug() << "Requesting DcmAssoc";
        cond = ASC_requestAssociation(net, params, &assoc);
        if (cond.good())
        {
            // Dump general information concerning the establishment of the network connection if required
            //
            qDebug() << "DcmAssoc accepted (max send PDV: " << assoc->sendPDVLength << ")";

            // Figure out which of the accepted presentation contexts should be used
            //
            presId = transferSyntax? ASC_findAcceptedPresentationContextID(assoc, abstractSyntax, transferSyntax):
                                     ASC_findAcceptedPresentationContextID(assoc, abstractSyntax);
            if (presId != 0)
            {
                // Main routine exit point is here.
                //
                return true;
            }

            cond = makeOFCondition(0, 1, OF_error, tr("Accepted presentation context ID not found").toUtf8());
            ASC_releaseAssociation(assoc);
            ASC_destroyAssociation(&assoc);
            assoc = nullptr;
        }
    }

    qDebug() << QString::fromUtf8(cond.text());
    return false;
}

QString DcmClient::cEcho(const QString &peerAet, const QString &peerAddress, int timeout)
{
    T_ASC_Association *assoc = nullptr;
    cond = ASC_requestAssociation(net, initAssocParams(peerAet, peerAddress, timeout), &assoc);
    QString ret;

    if (cond.good())
    {
        DIC_US msgId = assoc->nextMsgID++;
        DIC_US status = -1;

        cond = DIMSE_echoUser(assoc, msgId, 0 == timeout? DIMSE_BLOCKING: DIMSE_NONBLOCKING,
            timeout, &status, nullptr);
        if (cond.good())
        {
            ret = QString::fromLocal8Bit(DU_cstoreStatusString(status))
                .append(" ").append(QString::fromUtf8(assoc->params->theirImplementationVersionName));
        }

        ASC_releaseAssociation(assoc);
        ASC_destroyAssociation(&assoc);

        if (cond.good())
        {
            return ret;
        }
    }

    return QString::fromLocal8Bit(cond.text());
}

bool DcmClient::findSCU()
{
    if (!createAssociation(QSettings().value("dicom/mwl-server").toString()))
    {
        return false;
    }

    DcmDataset ds;
    BuildCFindDataSet(ds);

    T_DIMSE_C_FindRQ req;
    T_DIMSE_C_FindRSP rsp;
    bzero((char*)&req, sizeof(req));
    bzero((char*)&rsp, sizeof(rsp));
    strcpy(req.AffectedSOPClassUID, abstractSyntax);
    req.DataSetType = DIMSE_DATASET_PRESENT;
    req.Priority = DIMSE_PRIORITY_LOW;
    req.MessageID = assoc->nextMsgID++;

    // Finally conduct transmission of data
    //
    DcmDataset *statusDetail = nullptr;
    int tout = timeout();
    cond = DIMSE_findUser(assoc, presId, &req, &ds,
        loadCallback, static_cast<void*>(this),
        0 == tout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, tout,
        &rsp, &statusDetail);

    if (statusDetail != nullptr)
    {
        delete statusDetail;
    }

    return cond.good();
}

QString DcmClient::nCreateRQ(DcmDataset* dsPatient)
{
    if (!createAssociation(QSettings().value("dicom/mpps-server").toString()))
    {
        return nullptr;
    }

    DcmDataset dsNCreate;
    BuildNCreateDataSet(*dsPatient, dsNCreate);

#ifdef QT_DEBUG
    dsPatient->writeXML(std::cout << "------C-FIND------------" << std::endl);
    dsNCreate.writeXML(std::cout  << "------N-Create----------" << std::endl);
#endif

    T_DIMSE_Message req, rsp;
    bzero((char*)&req, sizeof(req));
    bzero((char*)&rsp, sizeof(rsp));

    req.CommandField = DIMSE_N_CREATE_RQ;
    req.msg.NCreateRQ.MessageID = assoc->nextMsgID++;;
    strcpy(req.msg.NCreateRQ.AffectedSOPClassUID, abstractSyntax);
    req.msg.NCreateRQ.DataSetType = DIMSE_DATASET_PRESENT;

    // Send request to the server
    //
    cond = DIMSE_sendMessageUsingMemoryData(assoc, presId, &req, nullptr, &dsNCreate, nullptr, nullptr);
    if (cond.bad())
    {
        return nullptr;
    }

    // Receive response
    //
    int tout = timeout();
    cond = DIMSE_receiveCommand(assoc, 0 == tout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, tout, &presId, &rsp, nullptr);
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

    return QString::fromUtf8(rsp.msg.NCreateRSP.AffectedSOPInstanceUID);
}

bool DcmClient::nSetRQ(const char* seriesUID, DcmDataset* patientDs, const QString& sopInstance)
{
    if (!createAssociation(QSettings().value("dicom/mpps-server").toString()))
    {
        return nullptr;
    }

    DcmDataset nSetDs;
    BuildNSetDataSet(*patientDs, nSetDs, seriesUID, true);

#ifdef QT_DEBUG
    nSetDs.writeXML(std::cout << "------N-Set------------" << std::endl);
#endif

    T_DIMSE_Message req, rsp;
    bzero((char*)&req, sizeof(req));
    bzero((char*)&rsp, sizeof(rsp));

    req.CommandField = DIMSE_N_SET_RQ;
    req.msg.NSetRQ.MessageID = assoc->nextMsgID++;;
    strcpy(req.msg.NSetRQ.RequestedSOPClassUID, abstractSyntax);
    strcpy(req.msg.NSetRQ.RequestedSOPInstanceUID, sopInstance.toUtf8());
    req.msg.NSetRQ.DataSetType = DIMSE_DATASET_PRESENT;

    OFCondition cond = DIMSE_sendMessageUsingMemoryData(assoc, presId, &req, nullptr, &nSetDs, nullptr, nullptr);
    if (cond.bad()) return false;

    /* receive response */
    int tout = timeout();
    cond = DIMSE_receiveCommand(assoc, 0 == tout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, tout, &presId, &rsp, nullptr);
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

static void StoreUserCallback
    ( void *callbackData
    , T_DIMSE_StoreProgress *
    , T_DIMSE_C_StoreRQ *
   )
{
    auto pThis = static_cast<DcmClient*>(callbackData);

    if (pThis->progressDlg && pThis->progressDlg->wasCanceled())
    {
        pThis->abort();
    }
    qApp->processEvents();
}

bool DcmClient::cStoreRQ(DcmDataset* dset, const char* sopInstance)
{
    T_DIMSE_C_StoreRQ req;
    T_DIMSE_C_StoreRSP rsp;
    bzero((char*)&req, sizeof(req));
    bzero((char*)&rsp, sizeof(rsp));
    DcmDataset *statusDetail = nullptr;

    /* prepare the transmission of data */
    req.MessageID = assoc->nextMsgID++;
    strcpy(req.AffectedSOPClassUID, abstractSyntax);
    strcpy(req.AffectedSOPInstanceUID, sopInstance);
    req.DataSetType = DIMSE_DATASET_PRESENT;
    req.Priority = DIMSE_PRIORITY_LOW;

    /* finally conduct transmission of data */
    int tout = timeout();
    cond = DIMSE_storeUser(assoc, presId, &req, nullptr, dset, StoreUserCallback, this,
        0 == tout? DIMSE_BLOCKING: DIMSE_NONBLOCKING, tout, &rsp, &statusDetail);

    if (rsp.DimseStatus)
    {
        OFString err;
        if (!statusDetail || statusDetail->findAndGetOFString(DCM_ErrorComment, err).bad() || err.length() == 0)
        {
            err.assign(QString::number(rsp.DimseStatus).toUtf8());
        }

        cond = makeOFCondition(0, rsp.DimseStatus, OF_error, err.c_str());
    }

    delete statusDetail;
    return cond.good();
}

bool DcmClient::sendToServer(const QString& server, DcmDataset* dsPatient, const QString& seriesUID,
                             int seriesNumber, const QString& file, const QString& mimeType, int instanceNumber)
{
    E_TransferSyntax writeXfer = EXS_LittleEndianExplicit;

    char instanceUID[100] = {0};
    dcmGenerateUniqueIdentifier(instanceUID,  SITE_INSTANCE_UID_ROOT);

    qDebug() << mimeType << file;

    DcmDataset ds;
    if (mimeType.startsWith("application/"))
    {
        ds.putAndInsertString(DCM_Modality,       "OT");
        cond = readAndInsertGenericData(file, &ds, mimeType);
    }
    else
    {
        auto modality = GetFileExtAttribute(file, "modality");
        if (modality.isEmpty())
        {
            modality = QSettings().value("dicom/modality", DEFAULT_MODALITY).toString();
        }
        ds.putAndInsertString(DCM_Modality, modality.toUpper().toUtf8());
        cond = readAndInsertPixelData(file, &ds, writeXfer);
    }

    if (cond.bad())
    {
        qDebug() << QString::fromUtf8(cond.text());
        // cond.reset()
        //
        cond = EC_Normal;
        return true;
    }

    cond = ds.putAndInsertString(DCM_SOPInstanceUID, instanceUID);
    if (cond.bad())
        return false;
    cond = ds.putAndInsertString(DCM_InstanceNumber, QString::number(instanceNumber).toUtf8());
    if (cond.bad())
        return false;
    cond = ds.putAndInsertString(DCM_SeriesNumber, QString::number(seriesNumber).toUtf8());
    if (cond.bad())
        return false;

    BuildCStoreDataSet(*dsPatient, ds, seriesUID);

    //DcmFileFormat dcmff(&ds);
    //cond = dcmff.saveFile((file +".dcm").toLocal8Bit(), writeXfer);

    OFString sopClass;
    ds.findAndGetOFString(DCM_SOPClassUID, sopClass);
    abstractSyntax = sopClass.c_str();

    DcmXfer filexfer((E_TransferSyntax)writeXfer);
    if (!createAssociation(server.toUtf8(), filexfer.getXferID()))
    {
        return false;
    }

    cStoreRQ(&ds, instanceUID);

    ASC_releaseAssociation(assoc);
    ASC_destroyAssociation(&assoc);
    assoc = nullptr;

    return cond.good();
}

static void
translateDcmObjectToLatin(DcmObject *parent)
{
    DcmObject* obj = nullptr;
    while (obj = parent->nextInContainer(obj), obj != nullptr)
    {
        char *val = nullptr;
        if (obj->isaString() && obj->containsExtendedCharacters())
        {
            auto elm = dynamic_cast<DcmElement*>(obj);
            if (elm->getString(val).good())
            {
                auto str = QString::fromUtf8(val);
                auto tranlated = translateToLatin(str);
                //qDebug() << str << " => " << tranlated;
                elm->putString(tranlated.toUtf8().constData());
            }
        }

        if (!obj->isLeaf())
        {
            translateDcmObjectToLatin(obj);
        }
    }
}

bool DcmClient::sendToServer(QWidget *parent, DcmDataset *dsPatient, const QFileInfoList& listFiles)
{
    QProgressDialog pdlg(parent);
    progressDlg = &pdlg;
    QSettings settings;
    settings.beginGroup("dicom");

    bool result = true;
    auto translate  = settings.value("translate-cyrillic", DEFAULT_TRANSLATE_CYRILLIC).toBool();
    auto allowClips = settings.value("export-clips", DEFAULT_EXPORT_CLIPS_TO_DICOM).toBool();
    auto allowVideo = settings.value("export-video", DEFAULT_EXPORT_VIDEO_TO_DICOM).toBool();

    pdlg.setRange(0, listFiles.count());
    pdlg.setMinimumDuration(0);

    // Only single series for now
    //
    int imageSeriesNo = 1;
    int clipsSeriesNo = 2;
    int videoSeriesNo = 3;
    int pdfSeriesNo = 4;
    int seriesNo = 0;

    char seriesUID[100] = {0};
    dcmGenerateUniqueIdentifier(seriesUID, SITE_SERIES_UID_ROOT);
    int idx = strlen(seriesUID) - 1;

    if (translate)
    {
        dsPatient = static_cast<DcmDataset*>(dsPatient->clone());
        translateDcmObjectToLatin(dsPatient);
    }

    foreach (auto server, settings.value("storage-servers").toStringList())
    {
        for (auto i = 0; !pdlg.wasCanceled() && i < listFiles.count(); ++i)
        {
            auto file = listFiles[i];
            auto dir = file.dir();
            auto filePath = file.absoluteFilePath();
            if (QFile::exists(dir.filePath(file.completeBaseName())))
            {
                // Skip clip thumbnail
                //
                continue;
            }

            auto mimeType = TypeDetect(filePath);
            if (mimeType.startsWith("video/"))
            {
                auto thumbnailFileTemplate = QStringList("." + file.fileName() + ".*");
                auto isClip = !dir.entryList(thumbnailFileTemplate, QDir::Hidden | QDir::Files).isEmpty();

                if (isClip)
                {
                    if (!allowClips)
                    {
                        qDebug() << "Clip" << filePath << "skipped";
                        continue;
                    }
                    seriesNo = clipsSeriesNo;
                }
                else
                {
                    if (!allowVideo)
                    {
                        qDebug() << "Video log" << filePath << "skipped";
                        continue;
                    }
                    seriesNo = videoSeriesNo;
                }
            }
            else if (mimeType == "application/pdf")
            {
                seriesNo = pdfSeriesNo;
            }
            else
            {
                seriesNo = imageSeriesNo;
            }

            pdlg.setValue(i);
            pdlg.setLabelText(tr("Storing '%1' to '%2'").arg(file.fileName(), server));
            qApp->processEvents();

            seriesUID[idx] = '0' + seriesNo;
            if (!sendToServer(server, dsPatient, seriesUID, seriesNo, filePath, mimeType, i))
            {
                result = false;
                SetFileExtAttribute(filePath, "dicom-status", lastError());
                if (QMessageBox::Yes != QMessageBox::critical(&pdlg, parent->windowTitle(),
                      tr("Failed to send '%1' to '%2':\n%3\nContinue?").arg(filePath, server, lastError()),
                      QMessageBox::Yes, QMessageBox::No))
                {
                    // The user choose to cancel
                    //
                    break;
                }
            }
            else
            {
                SetFileExtAttribute(filePath, "dicom-status", "ok");
            }
        }
    }
    progressDlg = nullptr;

    if (translate)
    {
        delete dsPatient;
    }

    return result;
}
