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
#include <qscrollbar.h>

#include <QGlib/Type>
#include <QGlib/Connect>
#include <QGst/Bus>
#include <QGst/Parse>

static inline QBoxLayout::Direction bestDirection(const QSize &s)
{
	return s.width() >= s.height()? QBoxLayout::LeftToRight: QBoxLayout::TopToBottom;
}

static void Dump(QGst::ElementPtr elm)
{
    if (!elm)
    {
        qCritical() << " (null) ";
    }

    foreach(auto prop,  elm->listProperties())
    {
        const QString n = prop->name();
        const QGlib::Value v = elm->property(n.toUtf8());
        qCritical() << n << " = " << v.get<QString>();
    }

    QGst::ChildProxyPtr childProxy =  elm.dynamicCast<QGst::ChildProxy>();
    if (childProxy)
    {
        childProxy->data(NULL);
        auto cnt = childProxy->childrenCount();
        for (uint i = 0; i < cnt; ++i)
        {
            qCritical() << "==== CHILD ==== " << i;
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
    // TODO: implement fragment record
    //buttonsLayout->addWidget(btnRecord);

    //buttonsLayout->addSpacing(200);
    btnRecordAll = createButton(SLOT(onRecordAllClick()));

    btnRecordAll->setFlat(true);
    btnRecordAll->setFocusPolicy(Qt::NoFocus);
    onRecordAllClick();
    // TODO: implement record all
    //buttonsLayout->addWidget(btnRecordAll);

	outputLayout = new QBoxLayout(bestDirection(size()));

    videoOut = new QGst::Ui::VideoWidget();
	videoOut->setMinimumSize(160, 120);
    outputLayout->addWidget(videoOut);

    imageOut = new QLabel(tr("A bit of useful information shown here.\n"
		"The end user should see this message and be able to use the app without learning."));
	imageOut->setAlignment(Qt::AlignCenter);
	imageOut->setMinimumSize(160, 120);
    outputLayout->addWidget(imageOut);

	imageList = new QListWidget();
	imageList->setViewMode(QListView::IconMode);
	imageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	imageList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    imageList->setMinimumHeight(96);
    imageList->setMaximumHeight(96);
    imageList->setMovement(QListView::Static);
    imageList->setWrapping(false);

    auto mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addLayout(outputLayout);
	mainLayout->addWidget(imageList);

    setLayout(mainLayout);
    videoOut->watchPipeline(pipeline);

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
    videoFileName = settings.value("video-file", "'/video/'yyyy-MM-dd/HH-mm'/out.mpg'").toString();

    const QString pipeTemplate = settings.value("pipeline", "%1 ! tee name=splitter"
		" ! queue ! %2 splitter."
		" ! valve name=videovalve ! queue ! %3 ! filesink name=videosink splitter."
        " ! identity name=imagevalve signal-handoffs=true ! %4 ! multifilesink name=imagesink  post-messages=true splitter."
		).toString();
    const QString srcDef = settings.value("src", "autovideosrc").toString();
    const QString displaySinkDef = settings.value("display-sink", "autoconvert ! autovideosink").toString();
    const QString videoSinkDef   = settings.value("video-sink",  "ffenc_mpeg2video ! mpegtsmux").toString();
    const QString imageSinkDef   = settings.value("image-sink",  "videorate ! capsfilter caps=video/x-raw-yuv,framerate=1/3 ! ffmpegcolorspace ! pngenc snapshot=false").toString();

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

        videoValve = pl->getElementByName("videovalve");
        videoSink  = pl->getElementByName("videosink");
        imageValve = pl->getElementByName("imagevalve");
        imageSink  = pl->getElementByName("imagesink");

        splitter = pl->getElementByName("splitter");

		if (!videoValve || !videoSink || !imageValve || !imageSink || !splitter)
		{
			error(tr("The pipeline does not have all required elements"));
			pl.clear();
		}
        else
        {
            QGlib::connect(imageValve, "handoff", this, &MainWindow::onImageValveHandoff);

        }
    }

	return pl;
}

void MainWindow::onImageValveHandoff(const QGst::BufferPtr&)
{
    qCritical() << "handoff";
    static int a = 0;
    if (!a++) return;
    imageValve->setProperty("drop-probability", 1.0);
}

void MainWindow::error(const QString& msg)
{
	qCritical() << msg;
	QMessageBox msgBox(QMessageBox::Critical, windowTitle(), msg, QMessageBox::Ok, this);
	msgBox.exec();
}

void MainWindow::onBusMessage(const QGst::MessagePtr & message)
{
    switch (message->type())
    {
    case QGst::MessageStateChanged:
        if (message->source() == pipeline)
        {
            QGst::StateChangedMessagePtr m = message.staticCast<QGst::StateChangedMessage>();
            if (m->newState() == QGst::StatePaused)
            {
                //videoValve->setProperty("drop", 1);
                //imageValve->setProperty("drop-probability", 1);
                pipeline->setState(QGst::StatePlaying);

                //Dump(pipeline);
            }
			else if (m->newState() == QGst::StatePlaying)
			{
				// At this time the video output finally has a sink, so set it up now
				//
				auto videoSink = videoOut->videoSink();
				if (videoSink)
				{
					videoSink->setProperty("force-aspect-ratio", TRUE);
				}
			}
        }
        break;
    case QGst::MessageEos:
        qCritical() << "EOS???";
        break;
    case QGst::MessageError:
        error(message.staticCast<QGst::ErrorMessage>()->error());
        break;
    case QGst::MessageNewClock:
    case QGst::MessageStreamStatus:
    case QGst::MessageQos:
        break;
    case QGst::MessageElement:
		if (message->source() == imageSink)
        {
            QGst::ElementMessagePtr m = message.staticCast<QGst::ElementMessage>();
            const QGst::StructurePtr s = m->internalStructure();
            if (s && s->name() == "GstMultiFileSink")
            {
                imageValve->setProperty("drop-probability", 1.0);
				lastImageFile = s->value("filename").toString();
				imageTimer->start(500);
            }
            else
            {
                qCritical() << s->name();
            }
        }
        break;
    case QGst::MessageAsyncDone:
        qCritical() << "MessageAsyncDone";
        break;
    default:
        qCritical() << message->type();
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
        videoValve->setProperty("drop", 0);

        const QString currentImageFileName(now.toString(imageFileName));
        QDir::current().mkpath(QFileInfo(currentImageFileName).absolutePath());
        imageSink->setProperty("location", currentImageFileName);
        imageValve->setProperty("drop-probability", 0.0);
    }
    else
    {
        recording = false;
        updateRecordButton();
    }

    // start recording
    //
    pipeline->setState(running? QGst::StatePaused: QGst::StateNull);
}

void MainWindow::onSnapshotClick()
{
    imageValve->setProperty("drop-probability", 0.0);
}

void MainWindow::onRecordClick()
{
    recording = !recording;
    updateRecordButton();

    videoValve->setProperty("drop", !recording);
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
	qCritical() << lastImageFile << " " << imageList->sizePolicy().horizontalPolicy() << " " << imageList->sizeHint() ;

    QPixmap pm;
	if (pm.load(lastImageFile))
	{
        auto mini = pm.scaledToHeight(96);
		auto item = new QListWidgetItem(QIcon(mini), QString());
        item->setSizeHint(QSize(mini.width(), 96));
        imageList->setIconSize(QSize(mini.width(), 96));
        item->setToolTip(lastImageFile);
        imageList->addItem(item);
        imageList->scrollToItem(item);
		imageOut->setPixmap(pm);
	}
	else
	{
		imageOut->setText(tr("Failed to load image %1").arg(lastImageFile));
	}
	lastImageFile.clear();
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
	videoOut->setVisible(running);
}

void MainWindow::updateRecordButton()
{
    QIcon icon(recording? ":/buttons/pause": ":/buttons/play");
    QString strOnOff(recording? tr("Pause"): tr("Record"));
    btnRecord->setIcon(icon);
    btnRecord->setText(strOnOff);
}
