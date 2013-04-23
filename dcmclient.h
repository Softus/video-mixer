#ifndef DCMASSOC_H
#define DCMASSOC_H

#include <QObject>
#include <QString>

#define SITE_UID_ROOT                           "9.9.999.0.1" // TODO: request real one

#include <dcmtk/config/cfunix.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofcond.h>    /* for class OFCondition */

class T_ASC_Network;
class T_ASC_Association;
class T_ASC_Parameters;
class T_DIMSE_C_FindRQ;
class T_DIMSE_C_FindRSP;
class DcmDataset;

class DcmClient : public QObject
{
    Q_OBJECT

    T_ASC_Network* net;
    const char* abstractSyntax;
    unsigned char presId;
    T_ASC_Association *assoc;
    OFCondition cond;

public:
    explicit DcmClient(const char* abstractSyntax, QObject *parent = 0)
        : QObject(parent), net(nullptr), abstractSyntax(abstractSyntax), presId(0), assoc(nullptr)
    {
    }
    ~DcmClient();

    bool findSCU();

    // Returns UID for nSetRQ
    //
    QString nCreateRQ(DcmDataset* patientDs);

    // Returns seriesUID for sendToServer
    //
    QString nSetRQ(DcmDataset* patientDs, const QString& sopInstance);

    bool sendToServer(DcmDataset* patientDs, const QString& seriesUID, int seriesNumber,
        const QString file, int instanceNumber);

    QString lastError() const
    {
        return QString::fromLocal8Bit(cond.text());
    }

    void abort();

private:
    int timeout() const;
    T_ASC_Parameters* initAssocParams(const char * transferSyntax = nullptr);
    bool createAssociation(const char * transferSyntax = nullptr);
    bool cStoreRQ(DcmDataset* patientDs, int writeXfer, const char* sopInstance);
    static void loadCallback(void *callbackData,
        T_DIMSE_C_FindRQ* /*request*/, int /*responseCount*/,
        T_DIMSE_C_FindRSP* /*rsp*/, DcmDataset* responseIdentifiers);

    Q_DISABLE_COPY(DcmClient)

signals:
    void addRow(DcmDataset* responseIdentifiers);

public slots:
    
};

#endif // DCMASSOC_H
