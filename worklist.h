#ifndef WORKLIST_H
#define WORKLIST_H

#include <QDateTime>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QBoxLayout;
class QTableWidget;
class QTableWidgetItem;
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
    int           timeColumn;
    int           dateColumn;
    QList<int>    translateColumns;
    QAction*      actionLoad;
    QAction*      actionDetail;
    QAction*      actionStartStudy;
    QToolBar*     createToolBar();

    // DICOM
    //
    DcmClient* activeConnection;
    QString pendingSOPInstanceUID;

public:
    explicit Worklist(QWidget *parent = 0);

protected:
    virtual void closeEvent(QCloseEvent *);

signals:
    void startStudy(DcmDataset* patient);

public slots:
    void onLoadClick();
    void onShowDetailsClick();
    void onStartStudyClick();
    void onCellDoubleClicked(QTableWidgetItem* item);
    void onAddRow(DcmDataset* responseIdentifiers);
};

#endif // WORKLIST_H
