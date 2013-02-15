#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>

#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Buffer>
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
    QPushButton* btnSnapshot;
    QGst::Ui::VideoWidget* videoOut;
    QLabel* imageOut;

    QGst::ElementPtr splitter;

    QGst::ElementPtr imageValve;
    QGst::ElementPtr imageSink;
    QString imageFileName;

    QGst::ElementPtr videoValve;
    QGst::ElementPtr videoSink;
    QString videoFileName;

    QPushButton* createButton(const char *slot);
    void updateStartButton();
    void updateRecordButton();

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
