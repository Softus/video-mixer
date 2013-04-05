#include "mainwindow.h"
#ifdef WITH_DICOM
#include "worklist.h"
#endif
#include <QApplication>
#include <QResizeEvent>
#include <QFrame>
#include <QBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QMenu>
#include <QMenuBar>

#include <QGlib/Type>
#include <QGlib/Connect>
#include <QGst/Bus>
#include <QGst/Parse>
#include <QGst/Event>
#include <QGst/Clock>
#include <gst/gstdebugutils.h>

static inline QBoxLayout::Direction bestDirection(const QSize &s)
{
    return s.width() >= s.height()? QBoxLayout::LeftToRight: QBoxLayout::TopToBottom;
}

#ifdef QT_DEBUG

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

#endif

MainWindow::MainWindow(QWidget *parent) :
    BaseWidget(parent),
    running(false),
    recording(false),
    worklist(nullptr)
{
    QSettings settings;
    iconSize = settings.value("icon-size", iconSize).toInt();

    auto buttonsLayout = new QHBoxLayout();

#ifdef WITH_DICOM
    auto btnWorkList = createButton(":/buttons/show_worklist", tr("Show\n&work list"), SLOT(onShowWorkListClick()));
    buttonsLayout->addWidget(btnWorkList);
#endif

    btnStart = createButton(SLOT(onStartClick()));
    btnStart->setAutoDefault(true);
    buttonsLayout->addWidget(btnStart);

    btnSnapshot = createButton(":/buttons/camera", tr("Take\nsnapshot"), SLOT(onSnapshotClick()));
    buttonsLayout->addWidget(btnSnapshot);

    btnRecord = createButton(SLOT(onRecordClick()));
    buttonsLayout->addWidget(btnRecord);

    lblRecordAll = new QLabel();
    lblRecordAll->setMaximumHeight(iconSize + 8);
    buttonsLayout->addWidget(lblRecordAll, 0, Qt::AlignRight);

    outputLayout = new QBoxLayout(bestDirection(size()));

    displayWidget = new QGst::Ui::VideoWidget();
    displayWidget->setMinimumSize(320, 240);
    outputLayout->addWidget(displayWidget);

    imageOut = new QLabel(tr("To start a study press \"Start\" button.\n\n"
        "During the study you can take snapshots and save video clips."));
    imageOut->setAlignment(Qt::AlignCenter);
    imageOut->setMinimumSize(320, 240);
    outputLayout->addWidget(imageOut);

    // TODO: implement snapshot image list
    //
//    imageList = new QListWidget();
//    imageList->setViewMode(QListView::IconMode);
//    imageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    imageList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    imageList->setMinimumHeight(iconSize);
//    imageList->setMaximumHeight(iconSize);
//    imageList->setMovement(QListView::Static);
//    imageList->setWrapping(false);

    auto mainLayout = new QVBoxLayout();
    mainLayout->setMenuBar(createMenu());
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addLayout(outputLayout);
//    mainLayout->addWidget(imageList);

    setLayout(mainLayout);

    updateStartButton();
    updateRecordButton();
    updateRecordAll();

    QString profile = settings.value("profile").toString();
    setWindowTitle(QString().append(qApp->applicationName()).append(" - ").append(profile.isEmpty()? tr("Default"): profile));
}

MainWindow::~MainWindow()
{
    if (pipeline)
    {
        releasePipeline();
    }
}

QMenuBar* MainWindow::createMenu()
{
    auto mnuBar = new QMenuBar();
    auto mnu   = new QMenu(tr("&Menu"));

    mnu->addAction(tr("&About Qt"), qApp, SLOT(aboutQt()));
    mnu->addSeparator();
    auto rtpAction = mnu->addAction(tr("&Enable RTP streaming"), this, SLOT(toggleSetting()));
    rtpAction->setCheckable(true);
    rtpAction->setData("enable-rtp");

    auto fullVideoAction = mnu->addAction(tr("&Record entire study"), this, SLOT(toggleSetting()));
    fullVideoAction->setCheckable(true);
    fullVideoAction->setData("enable-video");

    mnu->addAction(tr("E&xit"), qApp, SLOT(quit()), Qt::ALT | Qt::Key_F4);
    connect(mnu, SIGNAL(aboutToShow()), this, SLOT(prepareSettingsMenu()));
    mnuBar->addMenu(mnu);

    auto profilesMenu = new QMenu(tr("&Profiles"), mnu);
    auto profileGroup = new QActionGroup(profilesMenu);
    auto defaultProfileAction = profilesMenu->addAction(tr("&Default"), this, SLOT(setProfile()));
    defaultProfileAction->setCheckable(true);
    profileGroup->addAction(defaultProfileAction);
    connect(profilesMenu, SIGNAL(aboutToShow()), this, SLOT(prepareProfileMenu()));
    mnuBar->addMenu(profilesMenu);

    mnuBar->show();
    return mnuBar;
}

/*

  Basic pipeline is:

                 [video src]
                     |
                     V
         +----[main splitter]-----+
         |            |           |
         V            V           V
 [image encoder]  [display]  [video encoder]
        |                         |
        V                         V
  [image writer]      +----[video splitter]----+
                      |           |            |
                      V           V            V
           [movie writer]  [clip writer]  [rtp sender]
*/


QGst::PipelinePtr MainWindow::createPipeline()
{
    QSettings settings;
    QGst::PipelinePtr pl;

    // Default values for all profiles
    //
    imageFileName = settings.value("image-file", "'/video/'yyyy-MM-dd/HH-mm'/image%03d.jpg'").toString();
     clipFileName = settings.value("clip-file",  "'/video/'yyyy-MM-dd/HH-mm'/clip%03d.mpg'").toString();
    videoFileName = settings.value("video-file", "'/video/'yyyy-MM-dd/HH-mm'/video.mpg'").toString();

    bool enableVideo = settings.value("enable-video").toBool();
    bool enableRtp   = settings.value("enable-rtp").toBool();

    QString srcDef            = settings.value("src", "autovideosrc").toString();
    QString displaySinkDef    = settings.value("display-sink",  "timeoverlay name=displayoverlay ! autovideosink name=displaysink async=0").toString();
    QString videoEncoderDef   = settings.value("video-encoder", "queue max-size-bytes=0 ! ffenc_mpeg2video name=videoencoder bitrate=2000").toString();
    QString videoSinkDef      = settings.value("video-sink",    "queue ! mpegtsmux ! filesink name=videosink sync=0 async=0").toString();
    QString rtpSinkDef        = settings.value("rtp-sink",      "queue ! rtph264pay ! udpsink name=rtpsink clients=127.0.0.1:5000 sync=0").toString();
    QString clipSinkDef       = settings.value("clip-sink",     "queue ! mpegtsmux name=clipmux ! multifilesink name=clipsink next-file=4 sync=0 async=0").toString();
    QString imageEncoderDef   = settings.value("image-encoder", "jpegenc").toString();
    QString imageSinkDef      = settings.value("image-sink",    "multifilesink name=imagesink post-messages=1 async=0 sync=0").toString();

    int videoEncoderBitrate = settings.value("bitrate").toInt();
    QString rtpClients = enableRtp? settings.value("rtp-clients").toString(): QString();

    // Switch to profile
    //
    settings.beginGroup(settings.value("profile").toString());

    videoEncoderBitrate = settings.value("bitrate", videoEncoderBitrate).toInt();
    if (enableRtp)
    {
        rtpClients = settings.value("rtp-clients", rtpClients).toString();
    }

    // Override a value with profile-specific one
    //
    imageFileName = settings.value("image-file", imageFileName).toString();
     clipFileName = settings.value("clip-file",   clipFileName).toString();
    videoFileName = settings.value("video-file", videoFileName).toString();

    srcDef            = settings.value("src", srcDef).toString();
    displaySinkDef    = settings.value("display-sink",  displaySinkDef).toString();
    videoEncoderDef   = settings.value("video-encoder", videoEncoderDef).toString();
    videoSinkDef      = settings.value("video-sink",    videoSinkDef).toString();
    rtpSinkDef        = settings.value("rtp-sink",      rtpSinkDef).toString();
    clipSinkDef       = settings.value("clip-sink",     clipSinkDef).toString();
    imageEncoderDef   = settings.value("image-encoder", imageEncoderDef).toString();
    imageSinkDef      = settings.value("image-sink",    imageSinkDef).toString();

    settings.endGroup();

    if (videoSinkDef.isEmpty())
    {
        enableVideo = false;
    }

    if (rtpSinkDef.isEmpty())
    {
        enableRtp = false;
    }

    QString pipe = QString().append(srcDef).append(" ! tee name=splitter");
    if (!displaySinkDef.isEmpty())
    {
        pipe.append(" ! ").append(displaySinkDef).append(" splitter.");
    }

    if (!videoEncoderDef.isEmpty() && (enableRtp || enableVideo || !clipSinkDef.isEmpty()))
    {
        pipe.append(" ! ").append(videoEncoderDef);
        if (enableRtp || enableVideo)
        {
            pipe.append(" ! tee name=videosplitter");
            if (enableVideo)
                pipe.append(" ! ").append(videoSinkDef).append(" videosplitter.");
            if (enableRtp)
                pipe.append(" ! ").append(rtpSinkDef).append(" videosplitter.");
        }

        if (!clipSinkDef.isEmpty())
        {
            pipe.append(" ! valve name=clipvalve ! ").append(clipSinkDef);
            if (enableRtp || enableVideo)
                pipe.append(" videosplitter.");
        }
        pipe.append(" splitter.");
    }

    if (!imageSinkDef.isEmpty())
        pipe.append(" ! identity name=imagevalve ! ").append(imageEncoderDef).append(" ! ").append(imageSinkDef).append(" splitter.");

    qCritical() << pipe;

    try
    {
        pl = QGst::Parse::launch(pipe).dynamicCast<QGst::Pipeline>();
    }
    catch (QGlib::Error ex)
    {
        errorGlib(pl, ex);
    }

    if (pl)
    {
        QGlib::connect(pl->bus(), "message", this, &MainWindow::onBusMessage);
        pl->bus()->addSignalWatch();
        displayWidget->watchPipeline(pl);

        displaySink = pl->getElementByName("displaysink");
        if (!displaySink)
            qCritical() << "Element displaysink not found";

        clipValve = pl->getElementByName("clipvalve");
        if (!clipValve)
            qCritical() << "Element clipvalve not found";

        if (enableVideo)
        {
            videoSink  = pl->getElementByName("videosink");
            if (!videoSink)
                qCritical() << "Element videosink not found";
        }

        clipSink  = pl->getElementByName("clipsink");
        if (!clipSink)
            qCritical() << "Element clipsink not found";

        imageValve = pl->getElementByName("imagevalve");
        if (!imageValve)
        {
            qCritical() << "Element imagevalve not found";
        }
        else
        {
            QGlib::connect(imageValve, "handoff", this, &MainWindow::onImageReady);
        }

        imageSink  = pl->getElementByName("imagesink");
        if (!imageSink)
            qCritical() << "Element imagesink not found";

        if (enableRtp)
        {
            rtpSink  = pl->getElementByName("rtpsink");
            if (!rtpSink)
            {
                qCritical() << "Element rtpSink not found";
            }
            else if (!rtpClients.isEmpty())
            {
                rtpSink->setProperty("clients", rtpClients);
            }
        }

        if (videoEncoderBitrate > 0)
        {
            auto videoEncoder = pl->getElementByName("videoencoder");
            if (!videoEncoder)
            {
                qCritical() << "Element videoencoder not found";
            }
            else
            {
                videoEncoder->setProperty("bitrate", videoEncoderBitrate);
            }
        }

//        QGlib::connect(pl->getElementByName("test"), "handoff", this, &MainWindow::onTestHandoff);

        auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS((GstBin*)(QGst::Pipeline::CType*)pl, details, qApp->applicationName().toUtf8());
    }

    return pl;
}

void MainWindow::releasePipeline()
{
    pipeline->setState(QGst::StateNull);
    pipeline->getState(NULL, NULL, -1);

    displaySink.clear();

    imageValve.clear();
    imageSink.clear();
    imageFileName.clear();

    clipValve.clear();
    clipSink.clear();
    clipFileName.clear();

    videoSink.clear();
    videoFileName.clear();

    rtpSink.clear();

    pipeline.clear();
    displayWidget->stopPipelineWatch();
}

void MainWindow::onImageReady(const QGst::BufferPtr& buf)
{
    qDebug() << "imageValve handoff " << buf->size() << " " << buf->timeStamp() << " " << buf->flags();
    imageValve->setProperty("drop-probability", 1.0);
}

void MainWindow::errorGlib(const QGlib::ObjectPtr& obj, const QGlib::Error& ex)
{
    const QString str = obj?
        QString().append(obj->property("name").toString()).append(" ").append(ex.message()):
        ex.message();
    error(str);
}

void MainWindow::onBusMessage(const QGst::MessagePtr& message)
{
//    qDebug() << message->typeName() << " " << message->source()->property("name").toString();

    switch (message->type())
    {
    case QGst::MessageStateChanged:
        onStateChangedMessage(message.staticCast<QGst::StateChangedMessage>());
        break;
    case QGst::MessageElement:
        onElementMessage(message.staticCast<QGst::ElementMessage>());
        break;
    case QGst::MessageError:
        errorGlib(message->source(), message.staticCast<QGst::ErrorMessage>()->error());
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

void MainWindow::onStateChangedMessage(const QGst::StateChangedMessagePtr& message)
{
//  qDebug() << message->oldState() << " => " << message->newState();

    // The pipeline will not start by itself since 2 of 3 renders are in NULL state
    // We need to kick off the display renderer to start the capture
    //
    if (message->oldState() == QGst::StateReady && message->newState() == QGst::StatePaused)
    {
        message->source().staticCast<QGst::Element>()->setState(QGst::StatePlaying);
    }
    else if (message->newState() == QGst::StateNull && message->source() == pipeline)
    {
        // The display area of the main window is filled with some garbage.
        // We need to redraw the contents.
        //
        update();
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

void MainWindow::onElementMessage(const QGst::ElementMessagePtr& message)
{
    const QGst::StructurePtr s = message->internalStructure();
    if (!s)
    {
        qDebug() << "Got empty QGst::MessageElement";
        return;
    }

    if (s->name() == "GstMultiFileSink" && message->source() == imageSink)
    {
        const QString fileName = s->value("filename").toString();
        QPixmap pm;

        auto lastBuffer = message->source()->property("last-buffer").get<QGst::BufferPtr>();
        bool ok = lastBuffer && pm.loadFromData(lastBuffer->data(), lastBuffer->size());

        // If we can not load from the buffer, try to load from the file
        //
        ok = ok || pm.load(fileName);

        if (ok)
        {
            imageOut->setPixmap(pm);
        }
        else
        {
            imageOut->setText(tr("Failed to load image %1").arg(fileName));
        }

        btnSnapshot->setEnabled(running);
        return;
    }

    if (s->name() == "prepare-xwindow-id")
    {
        // At this time the video output finally has a sink, so set it up now
        //
        auto sink = displayWidget->videoSink();
        if (sink)
        {
            sink->setProperty("force-aspect-ratio", TRUE);
            displayWidget->update();
        }
        return;
    }

    qDebug() << "Got unknown message " << s->name() << " from " << message->source()->property("name").toString();
}

void MainWindow::onStartClick()
{
    if (!running)
    {
        pipeline = createPipeline();
        if (pipeline)
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
                imageValve->setProperty("drop-probability", 1.0);
            }

            pipeline->setState(QGst::StatePaused);
            running = true;
        }
    }
    else
    {
        running = recording = false;
        updateRecordButton();
        releasePipeline();
    }

    updateStartButton();
    updateRecordAll();

    imageOut->clear();
    displayWidget->update();
}

void MainWindow::onSnapshotClick()
{
    // Turn the valve on for a while.
    //
    imageValve->setProperty("drop-probability", 0.0);
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
        displayOverlay->setProperty("text", recording? "R": "");
    }
}

void MainWindow::resizeEvent(QResizeEvent *evt)
{
    QWidget::resizeEvent(evt);
    outputLayout->setDirection(bestDirection(evt->size()));
}

void MainWindow::updateStartButton()
{
    QIcon icon(running? ":/buttons/stop": ":/buttons/add");
    QString strOnOff(running? tr("Stop\nstudy"): tr("Start\nstudy"));
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

void MainWindow::updateRecordAll()
{
    QString strOnOff(videoSink? tr("on"): tr("off"));
    lblRecordAll->setEnabled(videoSink);
    lblRecordAll->setToolTip(tr("Recording of entire study is %1").arg(strOnOff));
    if (running)
    {
        lblRecordAll->setPixmap(QIcon(":/buttons/record_on").pixmap(QSize(iconSize, iconSize)));
    }
    else
    {
        lblRecordAll->clear();
    }
}

void MainWindow::prepareProfileMenu()
{
    QSettings settings;
    const QString profile = settings.value("profile").toString();
    auto profilesMenu = static_cast<QMenu*>(sender());
    auto actions = profilesMenu->actions();
    auto defaultProfile = actions.at(0);
    auto profileGroup = defaultProfile->actionGroup();
    defaultProfile->setDisabled(running);
    defaultProfile->setChecked(profile.isEmpty());

    for (int  i = actions.size() - 1; i > 0; --i)
    {
        profilesMenu->removeAction(actions.at(i));
    }

    Q_FOREACH(auto grp, settings.childGroups())
    {
        auto profileAction = profilesMenu->addAction(grp, this, SLOT(setProfile()));
        profileAction->setCheckable(true);
        profileAction->setData(grp);
        profileAction->setChecked(profile == grp);
        profileAction->setDisabled(running);
        profileGroup->addAction(profileAction);
    }
}

void MainWindow::setProfile()
{
    auto action = static_cast<QAction*>(sender());
    auto profile = action->data().toString();

    QSettings().setValue("profile", profile);
    setWindowTitle(QString().append(qApp->applicationName()).append(" - ").append(profile.isEmpty()? tr("Default"): profile));
}

void MainWindow::prepareSettingsMenu()
{
    QSettings settings;

    auto menu = static_cast<QMenu*>(sender());
    Q_FOREACH(auto action, menu->actions())
    {
        const QString propName = action->data().toString();
        if (!propName.isEmpty())
        {
            action->setChecked(settings.value(propName).toBool());
            action->setDisabled(running);
            continue;
        }
    }
}

void MainWindow::toggleSetting()
{
    QSettings settings;
    const QString propName = static_cast<QAction*>(sender())->data().toString();
    bool enable = !settings.value(propName).toBool();
    settings.setValue(propName, enable);
}

#ifdef WITH_DICOM
void MainWindow::onShowWorkListClick()
{
    if (worklist == nullptr)
    {
        worklist = new Worklist();
    }
    worklist->showMaybeMaximized();
    worklist->activateWindow();
}
#endif
