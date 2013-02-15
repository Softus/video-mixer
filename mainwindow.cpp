#include "mainwindow.h"
#include <QApplication>
#include <QFrame>
#include <QBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QDir>
#include <QFileInfo>

#include <QGlib/Type>
#include <QGlib/Error>
#include <QGlib/Connect>
#include <QGst/Bus>
#include <QGst/Parse>

static const QSize minSize(96,96);

QPushButton* MainWindow::createButton(const char *slot)
{
    auto btn = new QPushButton();
    btn->setMinimumSize(minSize);
    btn->setIconSize(minSize);
    connect(btn, SIGNAL(clicked()), this, slot);

    return btn;
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
    QSettings settings;

    imageFileName = settings.value("image-file", "'/video/'yyyy-MM-dd/HH-mm'/image%03d.png'").toString();
    videoFileName = settings.value("video-file", "'/video/'yyyy-MM-dd/HH-mm'/out.mpg'").toString();

    const QString pipeTemplate = settings.value("pipeline", "%1 ! tee name=splitter ! %2 splitter. ! valve name=videovalve ! queue ! %3 splitter. ! valve name=imagevalve ! queue ! %4 splitter.").toString();
    const QString srcDef = settings.value("src", "autovideosrc").toString();
    const QString displaySinkDef = settings.value("display-sink", "queue ! autovideoconvert ! autovideosink").toString();
    const QString videoSinkDef    = settings.value("video-sink",  "mpeg2enc ! mpegtsmux ! filesink name=videosink").toString();
    const QString imageSinkDef   = settings.value("image-sink",   "videorate ! video/x-raw-yuv,framerate=1/2 ! colorspace ! pngenc snapshot=false ! multifilesink name=imagesink  post-messages=true").toString();

    const QString pipe = pipeTemplate.arg(srcDef, displaySinkDef, videoSinkDef, imageSinkDef);
    qCritical() << pipe;

    pipeline = QGst::Parse::launch(pipe).dynamicCast<QGst::Pipeline>();
    if (pipeline)
    {
        QGlib::connect(pipeline->bus(), "message", this, &MainWindow::onBusMessage);
        pipeline->bus()->addSignalWatch();
        Dump(pipeline);

        videoValve = pipeline->getElementByName("videovalve");
        videoSink  = pipeline->getElementByName("videosink");
        imageValve = pipeline->getElementByName("imagevalve");
        imageSink  = pipeline->getElementByName("imagesink");

        splitter = pipeline->getElementByName("splitter");
    }

    auto buttonsLayout = new QHBoxLayout();

    btnStart = createButton(SLOT(onStartClick()));
    buttonsLayout->addWidget(btnStart);

    btnSnapshot = createButton(SLOT(onSnapshotClick()));
    btnSnapshot->setIcon(QIcon(":/buttons/camera"));
    btnSnapshot->setText(tr("Take\nsnapshot"));
    buttonsLayout->addWidget(btnSnapshot);

    btnRecord = createButton(SLOT(onRecordClick()));
    // TODO: implement fragment record
    //buttonsLayout->addWidget(btnRecord);

    buttonsLayout->addSpacing(200);
    btnRecordAll = createButton(SLOT(onRecordAllClick()));

    btnRecordAll->setFlat(true);
    btnRecordAll->setFocusPolicy(Qt::NoFocus);
    onRecordAllClick();
    // TODO: implement record all
    //buttonsLayout->addWidget(btnRecordAll);

    auto outputLayout = new QHBoxLayout();

    videoOut = new QGst::Ui::VideoWidget();
    videoOut->setMinimumSize(minSize);
    outputLayout->addWidget(videoOut);

    imageOut = new QLabel();
    outputLayout->addWidget(imageOut);


    auto mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addLayout(outputLayout);

    setLayout(mainLayout);
    videoOut->watchPipeline(pipeline);

    updateStartButton();
    updateRecordButton();
}

MainWindow::~MainWindow()
{
    pipeline->setState(QGst::StateNull);
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
//                videoValve->setProperty("drop", 1);
//                imageValve->setProperty("drop", 1);
                pipeline->setState(QGst::StatePlaying);
            }
        }
        break;
    case QGst::MessageEos:
        qCritical() << "EOS???";
        break;
    case QGst::MessageError:
        qCritical() << message.staticCast<QGst::ErrorMessage>()->error();
        break;
    case QGst::MessageNewClock:
    case QGst::MessageStreamStatus:
        break;
    case QGst::MessageElement:
        {
            QGst::ElementMessagePtr m = message.staticCast<QGst::ElementMessage>();
            const QGst::StructurePtr s = m->internalStructure();
            if (s && s->name() == "GstMultiFileSink")
            {
                imageValve->setProperty("drop", 1);
                QPixmap p;
                p.load(s->value("filename").toString());
                imageOut->setPixmap(p);
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
        imageValve->setProperty("drop", 0);
    }
    else
    {
        recording = false;
        updateRecordButton();
        imageOut->clear();
    }

    // start recording
    //
    pipeline->setState(running? QGst::StatePaused: QGst::StateNull);
}

void MainWindow::onSnapshotClick()
{
    imageValve->setProperty("drop", 0);
}

void MainWindow::onRecordClick()
{
    recording = !recording;
    updateRecordButton();

    videoValve->setProperty("drop", !recording);
}

void MainWindow::onRecordAllClick()
{
    QIcon icon(":/buttons/record_on");

    recordAll = !recordAll;
    QString strOnOff(recordAll? tr("on"): tr("off"));
    btnRecordAll->setIcon(icon.pixmap(minSize, recordAll? QIcon::Normal: QIcon::Disabled));
    btnRecordAll->setToolTip(tr("Recording of entire examination is %1").arg(strOnOff));
    btnRecordAll->setText(tr("Record\nis %1").arg(strOnOff));
}

void MainWindow::updateStartButton()
{
    QIcon icon(running? ":/buttons/stop": ":/buttons/add");
    QString strOnOff(running? tr("Stop\nexamination"): tr("Start\nexamination"));
    btnStart->setIcon(icon);
    btnStart->setText(strOnOff);

    btnRecord->setDisabled(!running);
    btnSnapshot->setDisabled(!running);
}

void MainWindow::updateRecordButton()
{
    QIcon icon(recording? ":/buttons/pause": ":/buttons/play");
    QString strOnOff(recording? tr("Pause"): tr("Record"));
    btnRecord->setIcon(icon);
    btnRecord->setText(strOnOff);
}
