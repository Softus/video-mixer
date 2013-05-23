#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDir>
#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Pad>
#include <QGst/Buffer>
#include <QGst/Ui/VideoWidget>

#include <QGlib/Error>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QMenuBar;
class QResizeEvent;
class QBoxLayout;
class QListWidget;
class QLabel;
class QTimer;
class QToolButton;
class Worklist;
class DcmDataset;
QT_END_NAMESPACE

class MainWindow : public QWidget
{
    Q_OBJECT

    // UI
    //
    QLabel*      lblRecordAll;
    QToolButton* btnStart;
    QToolButton* btnRecord;
    QToolButton* btnSnapshot;

#ifdef WITH_DICOM
    Worklist*     worklist;
    QString       pendingSOPInstanceUID;
#endif
    QListWidget* imageList;
    QGst::Ui::VideoWidget* displayWidget;
    QDir         outputPath;

    QMenuBar* createMenu();
    void updateStartButton();
    void updateRecordButton();
    void updateRecordAll();

    // State machine
    //
    bool running;
    bool recording;

    // GStreamer pipeline
    //
    QGst::PipelinePtr pipeline;
    QGst::ElementPtr displaySink;
    QGst::ElementPtr imageValve;
    QGst::ElementPtr imageSink;
    QGst::ElementPtr clipValve;
    QGst::ElementPtr clipSink;
    QGst::ElementPtr videoValve;
    QGst::ElementPtr videoSink;
    QGst::ElementPtr rtpSink;

    QGst::PipelinePtr createPipeline();
    void releasePipeline();
    void restartPipeline();

    void onBusMessage(const QGst::MessagePtr& message);
    void onStateChangedMessage(const QGst::StateChangedMessagePtr& message);
    void onElementMessage(const QGst::ElementMessagePtr& message);

    void onImageReady(const QGst::BufferPtr&);
    void errorGlib(const QGlib::ObjectPtr& obj, const QGlib::Error& ex);
    void restartElement(const char* name);

#ifdef WITH_DICOM
    void sendToServer(DcmDataset* dset, const QString& seriesUID);
#endif

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:

private slots:
#ifdef WITH_DICOM
    void onShowWorkListClick();
#endif
    void onShowAboutClick();
    void onShowArchiveClick();
    void onShowSettingsClick();
    void onStartClick();
    void onSnapshotClick();
    void onRecordClick();
    void prepareSettingsMenu();
    void toggleSetting();
};

#endif // MAINWINDOW_H
