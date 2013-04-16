#ifndef WORKLIST_H
#define WORKLIST_H

#include "basewidget.h"
#include <QDateTime>

class QBoxLayout;
class QPushButton;
class QTableWidget;

class DcmDataset;
class DcmClient;

class Worklist : public BaseWidget
{
    Q_OBJECT

    // UI
    //
    QTableWidget* table;
    QDateTime     maxDate;
    QPushButton*  btnLoad;
    QPushButton*  btnDetail;

    // DICOM
    //
    DcmClient* activeConnection = nullptr;
    QString pendingSOPInstanceUID;

public:
    explicit Worklist(QWidget *parent = 0);
    ~Worklist();

    DcmDataset* getPatientDS();

protected:
    virtual void closeEvent(QCloseEvent *);

signals:
    
public slots:
    void onLoadClick();
    void onShowDetailsClick();
    void onAddRow(DcmDataset* responseIdentifiers);
};

#endif // WORKLIST_H
