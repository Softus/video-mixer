#ifndef WORKLIST_H
#define WORKLIST_H

#include "basewidget.h"
#include <QDateTime>

class QBoxLayout;
class QPushButton;
class QTableWidget;

struct T_ASC_Network;
struct T_ASC_Parameters;
struct T_ASC_Association;
struct T_DIMSE_C_FindRQ;
struct T_DIMSE_C_FindRSP;

class DcmDataset;
class DcmTagKey;
class OFCondition;

class Worklist : public BaseWidget
{
    Q_OBJECT

    // UI
    //
    QTableWidget*  table;
    QDateTime maxDate;
    QPushButton* btnLoad;

    // DICOM
    //
    T_ASC_Network* net = nullptr;
    T_ASC_Association* assoc = nullptr;
    QString pendingSOPInstanceUID;

    uchar createAssociationAndGetPresentationContextID(const char* abstractSyntax);

    bool findSCU(uchar presId, DcmDataset* dset);
    bool nCreateRQ(uchar presId, DcmDataset* dset);
    bool nSetRQ(uchar presId, DcmDataset* dset);

    void addRow(DcmDataset* dset);
    static void loadCallback(void *callbackData,
        T_DIMSE_C_FindRQ *request, int responseCount,
        T_DIMSE_C_FindRSP *rsp, DcmDataset *responseIdentifiers);

public:
    explicit Worklist(QWidget *parent = 0);
    ~Worklist();
    void errorOF(const OFCondition* cond);

protected:
    virtual void closeEvent(QCloseEvent *);

signals:
    
public slots:
    void onLoadClick();
    void onStartClick();
    void onDoneClick();
    void onAbortClick();

};

#endif // WORKLIST_H
