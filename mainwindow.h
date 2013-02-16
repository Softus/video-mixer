#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Pad>
#include <QGst/Buffer>
#include <QGst/Ui/VideoWidget>

#include <QGlib/Error>

class QMenuBar;
class QResizeEvent;
class QBoxLayout;
class QPushButton;
class QListWidget;
class QLabel;
class QTimer;

class MainWindow : public QWidget
{
    Q_OBJECT

	// State machine
	//
    bool running;
    bool recording;

	// UI
	//
	QBoxLayout*  outputLayout;
    QLabel*      lblRecordAll;
    QPushButton* btnStart;
    QPushButton* btnRecord;
    QPushButton* btnSnapshot;
    int          iconSize;

    QLabel*      imageOut;
	QTimer*      imageTimer;
	QString      lastImageFile;
//	QListWidget* imageList;
    QGst::Ui::VideoWidget* displayWidget;

	QMenuBar* createMenu();
    QPushButton* createButton(const char *slot);
    void updateStartButton();
    void updateRecordButton();
    void updateRecordAll();

	// GStreamer pipeline
	//
    QGst::PipelinePtr pipeline;
    QGst::ElementPtr displaySink;

    QGst::ElementPtr imageValve;
    QGst::ElementPtr imageSink;
    QString imageFileName;

    QGst::ElementPtr clipValve;
    QGst::ElementPtr clipSink;
    QString clipFileName;

    QGst::ElementPtr videoSink;
    QString videoFileName;

    QGst::PipelinePtr createPipeline();
	void releasePipeline();

    void onBusMessage(const QGst::MessagePtr& message);
    void onStateChangedMessage(const QGst::StateChangedMessagePtr& message);
    void onElementMessage(const QGst::ElementMessagePtr& message);

//    void onTestHandoff(const QGst::BufferPtr&);
    void error(const QGlib::ObjectPtr& obj, const QGlib::Error& ex);
    void restartElement(const char* name);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	void error(const QString& msg);

protected:
    // Event handlers
	//
	virtual void resizeEvent(QResizeEvent *evt);

private slots:
    void onStartClick();
    void onSnapshotClick();
    void onRecordClick();
	void onUpdateImage();
    void setProfile();
};

#endif // MAINWINDOW_H
