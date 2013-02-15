#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QPushButton>

#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Ui/VideoWidget>

class MainWindow : public QWidget
{
    bool recordAll;
    bool running;
    bool recording;

    QGst::PipelinePtr pipeline;

    QPushButton* btnRecordAll;
    QPushButton* btnStart;
    QPushButton* btnRecord;
    QGst::Ui::VideoWidget* videoOut;

    QPushButton* createButton(const char *slot);
    void onBusMessage(const QGst::MessagePtr & message);

    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void onStartClick();
    void onSnapshotClick();
    void onRecordClick();
    void onRecordAllClick();

};

#endif // MAINWINDOW_H
