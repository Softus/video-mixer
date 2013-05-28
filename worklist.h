#ifndef WORKLIST_H
#define WORKLIST_H

#include <QDateTime>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QBoxLayout;
class QTableWidget;
class QToolBar;
QT_END_NAMESPACE

class DcmDataset;
class DcmClient;

class Worklist : public QWidget
{
    Q_OBJECT

    // UI
    //
    QTableWidget* table;
    QDateTime     maxDate;
    QAction*      actionLoad;
    QAction*      actionDetail;
    QToolBar*     createToolBar();

    // DICOM
    //
    DcmClient* activeConnection;
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
    void onStartStudyClick();
    void onAddRow(DcmDataset* responseIdentifiers);
};

#endif // WORKLIST_H
