/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
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

#ifndef DCMASSOC_H
#define DCMASSOC_H

#include <QObject>
#include <QFileInfo>

#define HAVE_CONFIG_H
#include <dcmtk/config/osconfig.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofcond.h>    /* for class OFCondition */

struct T_ASC_Network;
struct T_ASC_Association;
struct T_ASC_Parameters;
struct T_DIMSE_C_FindRQ;
struct T_DIMSE_C_FindRSP;
class DcmDataset;

QT_BEGIN_NAMESPACE
class QProgressDialog;
QT_END_NAMESPACE

class DcmClient : public QObject
{
    Q_OBJECT

    T_ASC_Network* net;
    const char* abstractSyntax;
    unsigned char presId;
    T_ASC_Association *assoc;
    OFCondition cond;

public:
    DcmClient(const char* abstractSyntax = nullptr, QObject *parent = nullptr)
        : QObject(parent), net(nullptr), abstractSyntax(abstractSyntax), presId(0), assoc(nullptr), progressDlg(nullptr)
    {
    }
    ~DcmClient();

    QString cEcho(const QString& peerAet, const QString& peerAddress, int timeout);

    bool findSCU();

    // Returns UID for nSetRQ
    //
    QString nCreateRQ(DcmDataset* patientDs);

    // Returns seriesUID for sendToServer
    //
    bool nSetRQ(const char *seriesUID, DcmDataset* patientDs, const QString& sopInstance);

    void sendToServer(QWidget* parent, DcmDataset* patientDs, const QFileInfoList& files, const QString& seriesUID, bool force);

    bool sendToServer(const QString& server, DcmDataset* patientDs, const QString& seriesUID,
        int seriesNumber, const QString& file, int instanceNumber);

    QString lastError() const
    {
        return QString::fromLocal8Bit(cond.text());
    }

    void abort();
    QProgressDialog* progressDlg;

private:
    int timeout() const;
    T_ASC_Parameters* initAssocParams(const QString &server, const char* transferSyntax = nullptr);
    T_ASC_Parameters* initAssocParams(const QString& peerAet, const QString& peerAddress, int timeout, const char* transferSyntax = nullptr);
    bool createAssociation(const QString &server, const char* transferSyntax = nullptr);
    bool cStoreRQ(DcmDataset* patientDs, const char* sopInstance);
    static void loadCallback(void *callbackData,
        T_DIMSE_C_FindRQ* /*request*/, int /*responseCount*/,
        T_DIMSE_C_FindRSP* /*rsp*/, DcmDataset* responseIdentifiers);

    Q_DISABLE_COPY(DcmClient)

signals:
    void addRow(DcmDataset* responseIdentifiers);

public slots:
    
};

#endif // DCMASSOC_H
