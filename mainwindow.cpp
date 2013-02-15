#include "mainwindow.h"
#include <QApplication>
#include <QResizeEvent>
#include <QFrame>
#include <QBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <QGlib/Type>
#include <QGlib/Connect>
#include <QGst/Bus>
#include <QGst/Parse>
#include <QGst/Event>

static inline QBoxLayout::Direction bestDirection(const QSize &s)
{
	return s.width() >= s.height()? QBoxLayout::LeftToRight: QBoxLayout::TopToBottom;
}

//static void Dump(QGst::ElementPtr elm)
//{
//    if (!elm)
//    {
//        qDebug() << " (null) ";
//    }

//    foreach (auto prop, elm->listProperties())
//    {
//        const QString n = prop->name();
//        const QGlib::Value v = elm->property(n.toUtf8());
//        qDebug() << n << " = " << v.get<QString>();
//    }

//    QGst::ChildProxyPtr childProxy =  elm.dynamicCast<QGst::ChildProxy>();
//    if (childProxy)
//    {
//        childProxy->data(NULL);
//        auto cnt = childProxy->childrenCount();
//        for (uint i = 0; i < cnt; ++i)
//        {
//            qDebug() << "==== CHILD ==== " << i;
//            Dump(childProxy->childByIndex(i).dynamicCast<QGst::Element>());
//        }
//    }
//}

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    recordAll(false),
    running(false),
    recording(false)
{
    pipeline = createPipeline();

	imageTimer = new QTimer(this);
	imageTimer->setSingleShot(true);
	connect(imageTimer, SIGNAL(timeout()), this, SLOT(onUpdateImage()));

    auto buttonsLayout = new QHBoxLayout();
    btnStart = createButton(SLOT(onStartClick()));
	btnStart->setAutoDefault(true);
    buttonsLayout->addWidget(btnStart);

    btnSnapshot = createButton(SLOT(onSnapshotClick()));
    btnSnapshot->setIcon(QIcon(":/buttons/camera"));
    btnSnapshot->setText(tr("Take\nsnapshot"));
    buttonsLayout->addWidget(btnSnapshot);

    btnRecord = createButton(SLOT(onRecordClick()));
    buttonsLayout->addWidget(btnRecord);

    //buttonsLayout->addSpacing(200);
    btnRecordAll = createButton(SLOT(onRecordAllClick()));

    btnRecordAll->setFlat(true);
    btnRecordAll->setFocusPolicy(Qt::NoFocus);
    onRecordAllClick();
    // TODO: implement record all
    //buttonsLayout->addWidget(btnRecordAll);

	outputLayout = new QBoxLayout(bestDirection(size()));

    displayWidget = new QGst::Ui::VideoWidget();
    displayWidget->setMinimumSize(320, 240);
    outputLayout->addWidget(displayWidget);

    imageOut = new QLabel(tr("A bit of useful information shown here.\n"
		"The end user should see this message and be able to use the app without learning."));
	imageOut->setAlignment(Qt::AlignCenter);
    imageOut->setMinimumSize(320, 240);
    outputLayout->addWidget(imageOut);

    // TODO: implement snapshot image list
    //
//    imageList = new QListWidget();
//    imageList->setViewMode(QListView::IconMode);
//    imageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    imageList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    imageList->setMinimumHeight(96);
//    imageList->setMaximumHeight(96);
//    imageList->setMovement(QListView::Static);
//    imageList->setWrapping(false);

    auto mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addLayout(outputLayout);
//    mainLayout->addWidget(imageList);

    setLayout(mainLayout);
    displayWidget->watchPipeline(pipeline);

	btnStart->setEnabled(pipeline);
    updateStartButton();
    updateRecordButton();
}

MainWindow::~MainWindow()
{
	if (pipeline)
	{
		pipeline->setState(QGst::StateNull);
	}
}

QPushButton* MainWindow::createButton(const char *slot)
{
	const QSize minSize(96,96);
    auto btn = new QPushButton();
    btn->setMinimumSize(minSize);
    btn->setIconSize(minSize);
    connect(btn, SIGNAL(clicked()), this, slot);

    return btn;
}

QGst::PipelinePtr MainWindow::createPipeline()
{
    QSettings settings;
	QGst::PipelinePtr pl;

    imageFileName = settings.value("image-file", "'/video/'yyyy-MM-dd/HH-mm'/image%03d.png'").toString();
    videoFileName = settings.value("video-file", "'/video/'yyyy-MM-dd/HH-mm'/video%03d.mpg'").toString();

    const QString pipeTemplate = settings.value("pipeline", "%1 ! tee name=splitter"
        " ! %2 splitter."
        " ! valve name=videovalve ! %3 ! multifilesink name=videosink next-file=4 splitter."
        " ! valve name=imagevalve ! %4 ! multifilesink name=imagesink post-messages=1 splitter."
		).toString();
    const QString rtpClientsDef = settings.value("rtp-clients", "127.0.0.1:5000").toString();
    const QString srcDef = settings.value("src", "autovideosrc").toString();
    const QString displaySinkDef = settings.value("display-sink", "ffmpegcolorspace ! timeoverlay name=displayoverlay ! autovideosink name=displaysink").toString();
    const QString videoSinkDef   = settings.value("video-sink",  "queue ! videoscale ! video/x-raw-yuv,width=160,height=120 ! x264enc tune=zerolatency bitrate=1000 byte-stream=1 ! tee name=videosplitter ! rtph264pay ! udpsink clients=%1 videosplitter. queue ! matroskamux").toString().arg(rtpClientsDef);
    const QString imageSinkDef   = settings.value("image-sink",  "videorate drop-only=1 ! capsfilter caps=video/x-raw-yuv,framerate=1/1 ! ffmpegcolorspace ! pngenc snapshot=0").toString();

    const QString pipe = pipeTemplate.arg(srcDef, displaySinkDef, videoSinkDef, imageSinkDef);
    qCritical() << pipe;

	try
	{
		pl = QGst::Parse::launch(pipe).dynamicCast<QGst::Pipeline>();
//		Dump(pl);
	}
	catch (QGlib::Error ex)
	{
		error(ex);
	}

    if (pl)
    {
        QGlib::connect(pl->bus(), "message", this, &MainWindow::onBusMessage);
        pl->bus()->addSignalWatch();

        displaySink = pl->getElementByName("displaysink");
        videoValve = pl->getElementByName("videovalve");
        videoSink  = pl->getElementByName("videosink");
        imageValve = pl->getElementByName("imagevalve");
        imageSink  = pl->getElementByName("imagesink");

        splitter = pl->getElementByName("splitter");

        if (!displaySink || !videoValve || !videoSink || !imageValve || !imageSink || !splitter)
		{
			error(tr("The pipeline does not have all required elements"));
			pl.clear();
		}

//        QGlib::connect(pl->getElementByName("test"), "handoff", this, &MainWindow::onTestHandoff);
    }

	return pl;
}

//void MainWindow::onTestHandoff(const QGst::BufferPtr& buf)
//{
//    qDebug() << "handoff " << buf->size() << " " << buf->timeStamp() << " " << buf->flags();
//}

void MainWindow::error(const QString& msg)
{
	qCritical() << msg;
	QMessageBox msgBox(QMessageBox::Critical, windowTitle(), msg, QMessageBox::Ok, this);
	msgBox.exec();
}

void MainWindow::onBusMessage(const QGst::MessagePtr & message)
{
//    qDebug() << message->typeName() << " " << message->source()->property("name").toString();

    switch (message->type())
    {
    case QGst::MessageStateChanged:
        {
            QGst::StateChangedMessagePtr m = message.staticCast<QGst::StateChangedMessage>();
//            qDebug() << m->oldState() << " => " << m->newState();

            // The pipeline will not start by itself since 2 of 3 renders are in NULL state
            // We need to kick off the display renderer to start the capture
            //
            if (m->oldState() == QGst::StateReady && m->newState() == QGst::StatePaused)
            {
                message->source().staticCast<QGst::Element>()->setState(QGst::StatePlaying);
            }
            else if (m->newState() == QGst::StateNull && message->source() == pipeline)
            {
                // The display area of the main window is filled with some garbage.
                // We need to redraw the contents.
                //
                update();
            }
        }
        break;
    case QGst::MessageError:
        error(message.staticCast<QGst::ErrorMessage>()->error());
        break;
    case QGst::MessageInfo:
        qDebug() << message.staticCast<QGst::InfoMessage>()->error();
        break;
    case QGst::MessageWarning:
        qWarning() << message.staticCast<QGst::WarningMessage>()->error();
        break;
    case QGst::MessageEos:
    case QGst::MessageNewClock:
    case QGst::MessageStreamStatus:
    case QGst::MessageQos:
    case QGst::MessageAsyncDone:
        break;
    case QGst::MessageElement:
        {
            QGst::ElementMessagePtr m = message.staticCast<QGst::ElementMessage>();
            const QGst::StructurePtr s = m->internalStructure();
            if (!s)
            {
                qDebug() << "Got empty QGst::MessageElement";
            }
            else if (s->name() == "GstMultiFileSink" && message->source() == imageSink)
            {
                const QString fileName = s->value("filename").toString();

                // We want to get more then one image for the snapshot.
                // And drop all of them except the last one.
                //
                if (!lastImageFile.isEmpty())
                {
                    QFile::remove(lastImageFile);
                    imageValve->setProperty("drop", 1);
                }
                lastImageFile = fileName;
                imageTimer->start(500);
            }
            else if (s->name() == "prepare-xwindow-id")
            {
                // At this time the video output finally has a sink, so set it up now
                //
                auto videoSink = displayWidget->videoSink();
                if (videoSink)
                {
                    videoSink->setProperty("force-aspect-ratio", TRUE);
                    displayWidget->update();
                }
            }
            else
            {
                qDebug() << s->name();
            }
        }
        break;
        break;
    default:
        qDebug() << message->type();
        break;
    }
}

void MainWindow::onStartClick()
{
    running = !running;
    updateStartButton();
    imageOut->clear();

    if (running)
    {
        const QDateTime now = QDateTime::currentDateTime();
        const QString currentVideoFileName(now.toString(videoFileName));
        QDir::current().mkpath(QFileInfo(currentVideoFileName).absolutePath());
        videoSink->setProperty("location", currentVideoFileName);
        videoValve->setProperty("drop", 1);

        const QString currentImageFileName(now.toString(imageFileName));
        QDir::current().mkpath(QFileInfo(currentImageFileName).absolutePath());
        imageSink->setProperty("location", currentImageFileName);
        imageValve->setProperty("drop", 1);
    }
    else
    {
        recording = false;
        updateRecordButton();
    }

    // start/stop the recording
    //
    pipeline->setState(running? QGst::StatePaused: QGst::StateNull);
}

void MainWindow::onSnapshotClick()
{
    // Turn the valve on for a while.
    //
    imageValve->setProperty("drop", 0);
    //
    // Once an image will be ready, the valve will be turned off again.
    btnSnapshot->setEnabled(false);
}

void MainWindow::onRecordClick()
{
    recording = !recording;
    updateRecordButton();

    if (recording)
    {
        // Force output file to close
        //
        videoSink->setState(QGst::StateNull);

        // Manually increment video clip file name
        //
        videoSink->setProperty("index", videoSink->property("index").toInt() + 1);

        // And start to write a new file
        //
        videoSink->setState(QGst::StatePlaying);
    }

    videoValve->setProperty("drop", !recording);
    auto displayOverlay = pipeline->getElementByName("displayoverlay");
    if (displayOverlay)
    {
        displayOverlay->setProperty("text", recording? "R": "   ");
    }
}

void MainWindow::onRecordAllClick()
{
	const QSize minSize(96,96);
    QIcon icon(":/buttons/record_on");

    recordAll = !recordAll;
    QString strOnOff(recordAll? tr("on"): tr("off"));
    btnRecordAll->setIcon(icon.pixmap(minSize, recordAll? QIcon::Normal: QIcon::Disabled));
    btnRecordAll->setToolTip(tr("Recording of entire examination is %1").arg(strOnOff));
    btnRecordAll->setText(tr("Record\nis %1").arg(strOnOff));
}

void MainWindow::onUpdateImage()
{
    qDebug() << lastImageFile;

    QPixmap pm;
	if (pm.load(lastImageFile))
	{
//        auto mini = pm.scaledToHeight(96);
//        auto item = new QListWidgetItem(QIcon(mini), QString());
//        item->setSizeHint(QSize(mini.width(), 96));
//        imageList->setIconSize(QSize(mini.width(), 96));
//        item->setToolTip(lastImageFile);
//        imageList->addItem(item);
//        imageList->select(item);
		imageOut->setPixmap(pm);
	}
	else
	{
		imageOut->setText(tr("Failed to load image %1").arg(lastImageFile));
	}

    imageValve->setProperty("drop", 1);
    lastImageFile.clear();
    btnSnapshot->setEnabled(running);
}

void MainWindow::resizeEvent(QResizeEvent *evt)
{
	QWidget::resizeEvent(evt);
	outputLayout->setDirection(bestDirection(evt->size()));
}

void MainWindow::updateStartButton()
{
    QIcon icon(running? ":/buttons/stop": ":/buttons/add");
    QString strOnOff(running? tr("Stop\nexamination"): tr("Start\nexamination"));
    btnStart->setIcon(icon);
    btnStart->setText(strOnOff);

    btnRecord->setEnabled(running);
    btnSnapshot->setEnabled(running);
    displayWidget->setVisible(running);
}

void MainWindow::updateRecordButton()
{
    QIcon icon(recording? ":/buttons/pause": ":/buttons/play");
    QString strOnOff(recording? tr("Pause"): tr("Record"));
    btnRecord->setIcon(icon);
    btnRecord->setText(strOnOff);
}
