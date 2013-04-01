#ifndef WORKLIST_H
#define WORKLIST_H

#include "basewidget.h"
#include <QDateTime>

class QBoxLayout;
class QPushButton;
class QTableWidget;

struct T_ASC_Network;
class DcmDataset;
class DcmTagKey;
class OFCondition;
class DcmAssoc;

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
    DcmAssoc* activeAssoc = nullptr;
    QString pendingSOPInstanceUID;

public:
    explicit Worklist(QWidget *parent = 0);
    ~Worklist();

protected:
    virtual void closeEvent(QCloseEvent *);

signals:
    
public slots:
    void onLoadClick();
    void onStartClick();
    void onDoneClick();
    void onAbortClick();
    void onAddRow(DcmDataset* responseIdentifiers);
};

#endif // WORKLIST_H
