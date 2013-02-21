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
#include <QMenu>
#include <QMenuBar>

#include <QGlib/Type>
#include <QGlib/Connect>
#include <QGst/Bus>
#include <QGst/Parse>
#include <QGst/Event>
#include <QGst/Clock>
#include <gst/gstdebugutils.h>

#define DEFAULT_ICON_SIZE 96

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
    QWidget(parent),
    running(false),
    recording(false),
    iconSize(DEFAULT_ICON_SIZE)
{
    QSettings settings;
    iconSize = settings.value("icon-size", iconSize).toInt();

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

    lblRecordAll = new QLabel();
    lblRecordAll->setMaximumHeight(iconSize + 8);
    buttonsLayout->addWidget(lblRecordAll, 0, Qt::AlignRight);

    outputLayout = new QBoxLayout(bestDirection(size()));

    displayWidget = new QGst::Ui::VideoWidget();
    displayWidget->setMinimumSize(320, 240);
    outputLayout->addWidget(displayWidget);

    imageOut = new QLabel(tr("To start an examination press \"Start\" button.\n\n"
        "During the examination you can take snapshots and save video clips."));
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

    auto fullVideoAction = mnu->addAction(tr("&Record entire examination"), this, SLOT(toggleSetting()));
    fullVideoAction->setCheckable(true);
    fullVideoAction->setData("enable-video");

    mnu->addAction(tr("E&xit"), qApp, SLOT(quit()), Qt::ALT | Qt::Key_F4);
    connect(mnu, SIGNAL(aboutToShow()), this, SLOT(prepareSettingsMenu()));
    mnuBar->addMenu(mnu);

    auto profilesMenu = new QMenu(tr("&Profiles"), mnu);
    auto profileGroup = new QActionGroup(profilesMenu);
    auto defaultProfileAction = profilesMenu->addAction(tr("&Default"), this, SLOT(setProfile()));
    defaultProfileAction->setCheckable(true);
    defaultProfileAction->setChecked(true);
    profileGroup->addAction(defaultProfileAction);
    connect(profilesMenu, SIGNAL(aboutToShow()), this, SLOT(prepareProfileMenu()));
    mnuBar->addMenu(profilesMenu);

    mnuBar->show();
    return mnuBar;
}

QPushButton* MainWindow::createButton(const char *slot)
{
    auto btn = new QPushButton();
    btn->setMinimumSize(QSize(iconSize, iconSize + 8));
    btn->setIconSize(QSize(iconSize, iconSize));
    connect(btn, SIGNAL(clicked()), this, slot);

    return btn;
}

void MainWindow::error(const QString& msg)
{
    qCritical() << msg;
    QMessageBox msgBox(QMessageBox::Critical, windowTitle(), msg, QMessageBox::Ok, this);
    msgBox.exec();
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
    imageFileName = settings.value("image-file", "'/video/'yyyy-MM-dd/HH-mm'/image%03d.png'").toString();
     clipFileName = settings.value("clip-file",  "'/video/'yyyy-MM-dd/HH-mm'/clip%03d.mpg'").toString();
    videoFileName = settings.value("video-file", "'/video/'yyyy-MM-dd/HH-mm'/video.mpg'").toString();

    // Switch to profile
    //
    settings.beginGroup(settings.value("profile").toString());

    int videoEncoderBitrate = settings.value("bitrate").toInt();
    bool enableVideo = settings.value("enable-video").toBool();
    bool enableRtp = settings.value("enable-rtp").toBool();
    const QString rtpClients = enableRtp? settings.value("rtp-clients").toString(): QString();

    // Override a value with profile-specific one
    //
    imageFileName = settings.value("image-file", imageFileName).toString();
     clipFileName = settings.value("clip-file",   clipFileName).toString();
    videoFileName = settings.value("video-file", videoFileName).toString();

    const QString srcDef            = settings.value("src", "autovideosrc").toString();
    const QString displaySinkDef    = settings.value("display-sink",  "ffmpegcolorspace ! timeoverlay name=displayoverlay ! autovideosink name=displaysink async=0").toString();
    const QString videoEncoderDef   = settings.value("video-encoder", "timeoverlay ! ffmpegcolorspace ! x264enc name=videoencoder tune=zerolatency bitrate=1000 byte-stream=1").toString();
    const QString videoSinkDef      = settings.value("video-sink",    "queue ! matroskamux ! filesink name=videosink sync=0 async=0").toString();
    const QString rtpSinkDef        = settings.value("rtp-sink",      "queue ! rtph264pay config-interval=4 ! udpsink name=rtpsink clients=127.0.0.1:5000 async=0").toString();
    const QString clipSinkDef       = settings.value("clip-sink",     "queue ! stamp name=clipstamp ! video/x-h264,framerate=25/1 ! matroskamux name=clipmux ! multifilesink name=clipsink next-file=4 sync=0 async=0").toString();
    const QString imageEncoderDef   = settings.value("image-encoder", "videorate drop-only=1 ! video/x-raw-yuv,framerate=1/1 ! clockoverlay ! ffmpegcolorspace ! pngenc snapshot=0").toString();
    const QString imageSinkDef      = settings.value("image-sink",    "multifilesink name=imagesink post-messages=1 async=0 sync=0 ").toString();

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
        pipe.append(" ! valve name=imagevalve ! ").append(imageEncoderDef).append(" ! ").append(imageSinkDef).append(" splitter.");

    qCritical() << pipe;

    settings.endGroup();

    try
    {
        pl = QGst::Parse::launch(pipe).dynamicCast<QGst::Pipeline>();
    }
    catch (QGlib::Error ex)
    {
        error(pl, ex);
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
            qCritical() << "Element imagevalve not found";

        imageSink  = pl->getElementByName("imagesink");
        if (!imageSink)
            qCritical() << "Element imagesink not found";

        rtpSink  = pl->getElementByName("rtpsink");
        if (!rtpSink)
            qCritical() << "Element rtpSink not found";
        else if (!rtpClients.isEmpty())
        {
            rtpSink->setProperty("clients", rtpClients);
        }

        if (videoEncoderBitrate > 0)
        {
            auto videoEncoder = pl->getElementByName("videoencoder");
            if (!videoEncoder)
                qCritical() << "Element videoencoder not found";
            else
                videoEncoder->setProperty("bitrate", videoEncoderBitrate);
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

//void MainWindow::onTestHandoff(const QGst::BufferPtr& buf)
//{
//    qDebug() << "handoff " << buf->size() << " " << buf->timeStamp() << " " << buf->flags();
//}

void MainWindow::error(const QGlib::ObjectPtr& obj, const QGlib::Error& ex)
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
                imageValve->setProperty("drop", TRUE);
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

void MainWindow::onUpdateImage()
{
    qDebug() << lastImageFile;

    QPixmap pm;
    if (pm.load(lastImageFile))
    {
//        auto mini = pm.scaledToHeight(iconSize);
//        auto item = new QListWidgetItem(QIcon(mini), QString());
//        item->setSizeHint(QSize(mini.width(), iconSize));
//        imageList->setIconSize(QSize(mini.width(), iconSize));
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

void MainWindow::updateRecordAll()
{
    QString strOnOff(videoSink? tr("on"): tr("off"));
    lblRecordAll->setEnabled(videoSink);
    lblRecordAll->setToolTip(tr("Recording of entire examination is %1").arg(strOnOff));
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
    static_cast<QMenu*>(action->parent())->setDefaultAction(action);

    QSettings().setValue("profile", profile);

    setWindowTitle(QString().append(qApp->applicationName()).append(" - ").append(profile.isEmpty()? tr("Default"): profile));
}

void MainWindow::prepareSettingsMenu()
{
    QSettings settings;
    const QString profile = settings.value("profile").toString();

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
