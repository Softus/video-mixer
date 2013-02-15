#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Pad>
#include <QGst/Buffer>
#include <QGst/Ui/VideoWidget>

#include <QGlib/Error>

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
	bool recordAll;
    bool running;
    bool recording;

	// UI
	//
	QBoxLayout*  outputLayout;
    QPushButton* btnRecordAll;
    QPushButton* btnStart;
    QPushButton* btnRecord;
    QPushButton* btnSnapshot;

    QLabel*      imageOut;
	QTimer*      imageTimer;
	QString      lastImageFile;
//	QListWidget* imageList;
    QGst::Ui::VideoWidget* displayWidget;

    QPushButton* createButton(const char *slot);
    void updateStartButton();
    void updateRecordButton();

	// GStreamer pipeline
	//
    QGst::PipelinePtr pipeline;
    QGst::ElementPtr splitter;

    QGst::ElementPtr displaySink;

    QGst::ElementPtr imageValve;
    QGst::ElementPtr imageSink;
    QString imageFileName;

    QGst::ElementPtr videoValve;
    QGst::ElementPtr videoSink;
    QString videoFileName;

    QGst::PipelinePtr createPipeline();
    void onBusMessage(const QGst::MessagePtr & message);
//    void onTestHandoff(const QGst::BufferPtr&);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	void error(const QString& msg);
	inline void error(const QGlib::Error& ex) { error(ex.message()); }

protected:
    // Event handlers
	//
	virtual void resizeEvent(QResizeEvent *evt);

private slots:
    void onStartClick();
    void onSnapshotClick();
    void onRecordClick();
    void onRecordAllClick();
	void onUpdateImage();
};

#endif // MAINWINDOW_H
