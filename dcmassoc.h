#ifndef DCMASSOC_H
#define DCMASSOC_H

#include <QObject>
#include <QString>

#include <dcmtk/config/cfunix.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofcond.h>    /* for class OFCondition */

class T_ASC_Network;
class T_ASC_Association;
class T_ASC_Parameters;
class T_DIMSE_C_FindRQ;
class T_DIMSE_C_FindRSP;
class DcmDataset;

class DcmAssoc : public QObject
{
    Q_OBJECT

    T_ASC_Network* net;
    const char* abstractSyntax;
    unsigned char presId;
    T_ASC_Association *assoc;
    OFCondition cond;

public:
    explicit DcmAssoc(T_ASC_Network* net, const char* abstractSyntax, QObject *parent = 0)
        : QObject(parent), net(net), abstractSyntax(abstractSyntax), presId(0), assoc(nullptr)
    {
    }
    ~DcmAssoc();

    bool findSCU(DcmDataset* dset);
    QString nCreateRQ(DcmDataset* dset);
    bool nSetRQ(DcmDataset* dset, QString pendingSOPInstanceUID);

    QString getLastError() const
    {
        return QString::fromLocal8Bit(cond.text());
    }

    void abort();

private:
    T_ASC_Parameters* initAssocParams();
    bool createAssociation();
    static void loadCallback(void *callbackData,
        T_DIMSE_C_FindRQ* /*request*/, int /*responseCount*/,
        T_DIMSE_C_FindRSP* /*rsp*/, DcmDataset* responseIdentifiers);

    Q_DISABLE_COPY(DcmAssoc)

signals:
    void addRow(DcmDataset* responseIdentifiers);

public slots:
    
};

#endif // DCMASSOC_H
