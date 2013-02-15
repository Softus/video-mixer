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
#include <QGst/Clock>

static inline QBoxLayout::Direction bestDirection(const QSize &s)
{
	return s.width() >= s.height()? QBoxLayout::LeftToRight: QBoxLayout::TopToBottom;
}

static void Dump(QGst::ElementPtr elm)
{
    if (!elm)
    {
        qDebug() << " (null) ";
		return;
    }

    foreach (auto prop, elm->listProperties())
    {
        const QString n = prop->name();
        const QGlib::Value v = elm->property(n.toUtf8());
		switch (v.type().fundamental())
		{
		case QGlib::Type::Boolean:
	        qDebug() << n << " = " << v.get<bool>();
			break;
		case QGlib::Type::Float:
		case QGlib::Type::Double:
	        qDebug() << n << " = " << v.get<double>();
			break;
		case QGlib::Type::Enum:
		case QGlib::Type::Flags:
		case QGlib::Type::Int:
		case QGlib::Type::Uint:
	        qDebug() << n << " = " << v.get<int>();
			break;
		case QGlib::Type::Long:
		case QGlib::Type::Ulong:
	        qDebug() << n << " = " << v.get<long>();
			break;
		case QGlib::Type::Int64:
		case QGlib::Type::Uint64:
	        qDebug() << n << " = " << v.get<qint64>();
			break;
		default:
	        qDebug() << n << " = " << v.get<QString>();
			break;
		}
    }

    QGst::ChildProxyPtr childProxy =  elm.dynamicCast<QGst::ChildProxy>();
    if (childProxy)
    {
        childProxy->data(NULL);
        auto cnt = childProxy->childrenCount();
        for (uint i = 0; i < cnt; ++i)
        {
            qDebug() << "==== CHILD ==== " << i;
            Dump(childProxy->childByIndex(i).dynamicCast<QGst::Element>());
        }
    }
}

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
     clipFileName = settings.value("clip-file",  "'/video/'yyyy-MM-dd/HH-mm'/clip%03d.mpg'").toString();
    videoFileName = settings.value("video-file", "'/video/'yyyy-MM-dd/HH-mm'/video.mpg'").toString();

    const QString pipeTemplate = settings.value("pipeline", "%1 ! tee name=splitter"
        " ! %2 splitter."
        " ! %3 ! tee name=videosplitter"
            " ! queue ! matroskamux ! filesink name=videosink videosplitter."
//            " ! rtph264pay ! udpsink clients=127.0.0.1:5000 videosplitter."
            " ! valve name=clipvalve ! stamp name=clipstamp ! queue ! matroskamux name=clipmux ! multifilesink name=clipsink next-file=4 async=0 videosplitter. splitter."
        " ! valve name=imagevalve ! %4 ! multifilesink name=imagesink post-messages=1 async=0 splitter."
		).toString();
    const QString srcDef = settings.value("src", "autovideosrc").toString();
    const QString displaySinkDef = settings.value("display-sink", "ffmpegcolorspace ! timeoverlay name=displayoverlay ! autovideosink name=displaysink sync=0").toString();
    const QString videoEncoderDef   = settings.value("video-encoder",  "timeoverlay ! ffmpegcolorspace ! x264enc tune=zerolatency bitrate=1000 byte-stream=1").toString();
    const QString imageEncoderDef   = settings.value("image-encoder",  "videorate drop-only=1 ! video/x-raw-yuv,framerate=1/1 ! ffmpegcolorspace ! clockoverlay ! pngenc snapshot=0").toString();

    const QString pipe = pipeTemplate.arg(srcDef, displaySinkDef, videoEncoderDef, imageEncoderDef);
    qCritical() << pipe;

	try
	{
		pl = QGst::Parse::launch(pipe).dynamicCast<QGst::Pipeline>();
        //Dump(pl);
	}
	catch (QGlib::Error ex)
	{
		error(pl, ex);
	}

    if (pl)
    {
        QGlib::connect(pl->bus(), "message", this, &MainWindow::onBusMessage);
        pl->bus()->addSignalWatch();

        displaySink = pl->getElementByName("displaysink");
		if (!displaySink)
			qCritical() << "Element displaysink not found";

        clipValve = pl->getElementByName("clipvalve");
        if (!clipValve)
            qCritical() << "Element clipvalve not found";

		videoSink  = pl->getElementByName("videosink");
		if (!videoSink)
			qCritical() << "Element videosink not found";

        clipSink  = pl->getElementByName("clipsink");
        if (!clipSink)
            qCritical() << "Element clipsink not found";

        imageValve = pl->getElementByName("imagevalve");
		if (!imageValve)
			qCritical() << "Element imagevalve not found";

        imageSink  = pl->getElementByName("imagesink");
		if (!imageSink)
			qCritical() << "Element imagesink not found";

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

void MainWindow::error(const QGlib::ObjectPtr& obj, const QGlib::Error& ex)
{
	const QString str = obj?
		QString().append(obj->property("name").toString()).append(" ").append(ex.message()):
		ex.message();
	error(str);
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
                    imageValve->setProperty("drop", TRUE);
                }
                lastImageFile = fileName;
                imageTimer->start(500);
            }
            else if (s->name() == "prepare-xwindow-id")
            {
                // At this time the video output finally has a sink, so set it up now
                //
                auto sink = displayWidget->videoSink();
                if (sink)
                {
                    sink->setProperty("force-aspect-ratio", TRUE);
                    displayWidget->update();
                }
            }
            else
            {
                qDebug() << s->name();
            }
        }
        break;
    case QGst::MessageError:
		error(message->source(), message.staticCast<QGst::ErrorMessage>()->error());
        break;
#ifdef QT_DEBUG
    case QGst::MessageInfo:
        qDebug() << message->source()->property("name").toString() << " " << message.staticCast<QGst::InfoMessage>()->error();
        break;
    case QGst::MessageWarning:
        qDebug() << message->source()->property("name").toString() << " " << message.staticCast<QGst::WarningMessage>()->error();
        break;
    case QGst::MessageEos:
    case QGst::MessageNewClock:
    case QGst::MessageStreamStatus:
    case QGst::MessageQos:
    case QGst::MessageAsyncDone:
        break;
    default:
        qDebug() << message->type();
        break;
#else
    default: // Make the compiler happy
        break;
#endif
    }
}

void MainWindow::restartElement(const char* name)
{
    auto elm = pipeline->getElementByName(name);
    if (!elm)
    {
        qDebug() << "Element " << name << " not found";
        return;
    }

    elm->setState(QGst::StateReady);
    elm->getState(NULL, NULL, -1);
    elm->setState(QGst::StatePlaying);
}

void MainWindow::onStartClick()
{
    running = !running;
    updateStartButton();
    imageOut->clear();

    if (running)
    {
        const QDateTime now = QDateTime::currentDateTime();
		if (videoSink)
		{
			const QString currentVideoFileName(now.toString(videoFileName));
			QDir::current().mkpath(QFileInfo(currentVideoFileName).absolutePath());
			videoSink->setProperty("location", currentVideoFileName);
		}

        if (clipSink)
        {
            const QString currentClipFileName(now.toString(clipFileName));
            QDir::current().mkpath(QFileInfo(currentClipFileName).absolutePath());
            clipSink->setProperty("location", currentClipFileName);
            clipSink->setProperty("index", 0);
        }

		if (imageSink)
		{
			const QString currentImageFileName(now.toString(imageFileName));
			QDir::current().mkpath(QFileInfo(currentImageFileName).absolutePath());
			imageSink->setProperty("location", currentImageFileName);
	        imageSink->setProperty("index", 0);
		}

        if (clipValve)
        {
            clipValve->setProperty("drop", TRUE);
        }

        if (imageValve)
        {
			imageValve->setProperty("drop", TRUE);
        }
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
    imageValve->setProperty("drop", FALSE);
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
        restartElement("clipstamp");
        restartElement("clipmux");

        // Manually increment video clip file name
        //
        clipSink->setState(QGst::StateNull);
        clipSink->setProperty("index", clipSink->property("index").toInt() + 1);
        clipSink->setState(QGst::StatePlaying);
    }
    else
    {
        clipValve->sendEvent(QGst::FlushStartEvent::create());
        clipValve->sendEvent(QGst::FlushStopEvent::create());
    }

    clipValve->setProperty("drop", !recording);

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

    btnRecord->setEnabled(running && clipSink);
	btnSnapshot->setEnabled(running && imageSink);
	displayWidget->setVisible(running && displaySink);
}

void MainWindow::updateRecordButton()
{
    QIcon icon(recording? ":/buttons/pause": ":/buttons/play");
    QString strOnOff(recording? tr("Pause"): tr("Record"));
    btnRecord->setIcon(icon);
    btnRecord->setText(strOnOff);
}
