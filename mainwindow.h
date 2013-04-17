#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDir>
#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Pad>
#include <QGst/Buffer>
#include <QGst/Ui/VideoWidget>

#include <QGlib/Error>

#include "basewidget.h"

class QMenuBar;
class QResizeEvent;
class QBoxLayout;
class QPushButton;
class QListWidget;
class QLabel;
class QTimer;
class Worklist;

class MainWindow : public BaseWidget
{
    Q_OBJECT

    // UI
    //
    QBoxLayout*  outputLayout;
    QLabel*      lblRecordAll;
    QPushButton* btnStart;
    QPushButton* btnRecord;
    QPushButton* btnSnapshot;

#ifdef WITH_DICOM
    Worklist*     worklist;
    QString       pendingSOPInstanceUID;
#endif
    QLabel*      imageOut;
//	QListWidget* imageList;
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
    QGst::ElementPtr videoSink;
    QGst::ElementPtr rtpSink;

    QGst::PipelinePtr createPipeline();
    void releasePipeline();

    void onBusMessage(const QGst::MessagePtr& message);
    void onStateChangedMessage(const QGst::StateChangedMessagePtr& message);
    void onElementMessage(const QGst::ElementMessagePtr& message);

    void onImageReady(const QGst::BufferPtr&);
    void errorGlib(const QGlib::ObjectPtr& obj, const QGlib::Error& ex);
    void restartElement(const char* name);

#ifdef WITH_DICOM
    void sendToServer();
#endif

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    // Event handlers
    //
    virtual void resizeEvent(QResizeEvent *evt);

private slots:
#ifdef WITH_DICOM
    void onShowWorkListClick();
#endif
    void onStartClick();
    void onSnapshotClick();
    void onRecordClick();
    void setProfile();
    void prepareSettingsMenu();
    void prepareProfileMenu();
    void toggleSetting();
};

#endif // MAINWINDOW_H
