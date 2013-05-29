#include "mainwindow.h"
#include "product.h"
#include "aboutdialog.h"
#include "archivewindow.h"
#include "qwaitcursor.h"
#include "settings.h"
#include "videosettings.h"

#ifdef WITH_DICOM
#include "worklist.h"
#include "startstudydialog.h"
#include "dcmclient.h"
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcuid.h>
#else
#include "patientdialog.h"
#endif

#include <QApplication>
#include <QBoxLayout>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QProgressDialog>
#include <QResizeEvent>
#include <QSettings>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

#include <QGlib/Connect>
#include <QGlib/Type>
#include <QGst/Bus>
#include <QGst/Clock>
#include <QGst/Event>
#include <QGst/Parse>
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
    QWidget(parent),
    archiveWindow(nullptr),
#ifdef WITH_DICOM
    worklist(nullptr),
#endif
    imageNo(0),
    clipNo(0),
    studyNo(0),
    running(false),
    recording(false)
{
    QVBoxLayout* layoutMain = new QVBoxLayout();
    layoutMain->setMenuBar(createMenuBar());
    layoutMain->addWidget(createToolBar());

    displayWidget = new QGst::Ui::VideoWidget();
    displayWidget->setMinimumSize(720, 576);
    layoutMain->addWidget(displayWidget);

    imageList = new QListWidget();
    imageList->setViewMode(QListView::IconMode);
    imageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    imageList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    imageList->setMinimumHeight(140);
    imageList->setMaximumHeight(140);
    imageList->setIconSize(QSize(128,128));
    imageList->setMovement(QListView::Static);
    imageList->setWrapping(false);
    layoutMain->addWidget(imageList);

    setLayout(layoutMain);

    updateStartButton();
    updateRecordButton();
    updateRecordAll();

    updatePipeline();
}

MainWindow::~MainWindow()
{
    if (pipeline)
    {
        releasePipeline();
    }

    delete archiveWindow;
    archiveWindow = nullptr;

#ifdef WITH_DICOM
    delete worklist;
    worklist = nullptr;
#endif
}

void MainWindow::closeEvent(QCloseEvent *evt)
{
    if (archiveWindow)
    {
        archiveWindow->close();
    }
#ifdef WITH_DICOM
    if (worklist)
    {
        worklist->close();
    }
#endif
    QWidget::closeEvent(evt);
}


QMenuBar* MainWindow::createMenuBar()
{
    auto mnuBar = new QMenuBar();
    auto mnu    = new QMenu(tr("&Menu"));

    auto actionAbout = mnu->addAction(tr("A&bout " PRODUCT_FULL_NAME).append(0x2026), this, SLOT(onShowAboutClick()));
    actionAbout->setMenuRole(QAction::AboutRole);
    mnu->addAction(tr("&Archive"), this, SLOT(onShowArchiveClick()), Qt::Key_F2);
#ifdef WITH_DICOM
    mnu->addAction(tr("&Worlkist"), this, SLOT(onShowWorklistClick()), Qt::Key_F3);
#endif
    mnu->addSeparator();
    auto actionRtp = mnu->addAction(tr("&Enable RTP streaming"), this, SLOT(toggleSetting()));
    actionRtp->setCheckable(true);
    actionRtp->setData("enable-rtp");

    auto actionFullVideo = mnu->addAction(tr("&Record entire study"), this, SLOT(toggleSetting()));
    actionFullVideo->setCheckable(true);
    actionFullVideo->setData("enable-video");

    auto actionPreferences = mnu->addAction(tr("&Preferences").append(0x2026), this, SLOT(onShowSettingsClick()), Qt::Key_F9);
    actionPreferences->setMenuRole(QAction::PreferencesRole);
    mnu->addSeparator();
    auto actionExit = mnu->addAction(tr("E&xit"), qApp, SLOT(quit()), Qt::ALT | Qt::Key_F4);
    actionExit->setMenuRole(QAction::QuitRole);

    connect(mnu, SIGNAL(aboutToShow()), this, SLOT(prepareSettingsMenu()));
    mnuBar->addMenu(mnu);

    mnuBar->show();
    return mnuBar;
}

QToolBar* MainWindow::createToolBar()
{
    QToolBar* bar = new QToolBar(tr("main"));

    btnStart = new QToolButton();
    btnStart->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnStart->setFocusPolicy(Qt::NoFocus);
    btnStart->setMinimumWidth(120);
    connect(btnStart, SIGNAL(clicked()), this, SLOT(onStartClick()));
    bar->addWidget(btnStart);

    btnSnapshot = new QToolButton();
    btnSnapshot->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnSnapshot->setFocusPolicy(Qt::NoFocus);
    btnSnapshot->setIcon(QIcon(":/buttons/camera"));
    btnSnapshot->setText(tr("&Take snapshot"));
    btnSnapshot->setMinimumWidth(120);
    connect(btnSnapshot, SIGNAL(clicked()), this, SLOT(onSnapshotClick()));
    bar->addWidget(btnSnapshot);

    btnRecord = new QToolButton();
    btnRecord->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnRecord->setFocusPolicy(Qt::NoFocus);
    btnRecord->setMinimumWidth(120);
    connect(btnRecord, SIGNAL(clicked()), this, SLOT(onRecordClick()));
    bar->addWidget(btnRecord);

    QWidget* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    bar->addWidget(spacer);

    lblRecordAll = new QLabel;
    lblRecordAll->setMaximumSize(bar->iconSize());
    lblRecordAll->setMinimumWidth(32);
    bar->addWidget(lblRecordAll);

#ifdef WITH_DICOM
    QAction* actionWorklist = bar->addAction(QIcon(":/buttons/show_worklist"), nullptr, this, SLOT(onShowWorkListClick()));
    actionWorklist->setToolTip(tr("Show &work list"));
#endif

    QAction* actionArchive = bar->addAction(QIcon(":/buttons/database"), nullptr, this, SLOT(onShowArchiveClick()));
    actionArchive->setToolTip(tr("Show studies archive"));
    actionSettings = bar->addAction(QIcon(":/buttons/settings"), nullptr, this, SLOT(onShowSettingsClick()));
    actionSettings->setToolTip(tr("Edit settings"));
    QAction* actionAbout = bar->addAction(QIcon(":/buttons/about"), nullptr, this, SLOT(onShowAboutClick()));
    actionAbout->setToolTip(tr("About berillyum"));

    return bar;
}

/*

  The pipeline is:

                 [video src]
                     |
                     V
         +----[main splitter]-----+
         |            |           |
  [image valve]       |       [video valve]
         |            |           |
         V            V           V
 [image encoder]  [display]  [video encoder]
        |                         |
        V                         V
  [image writer]      +----[video splitter]----+
                      |           |            |
                      V           V            V
           [movie writer]  [clip valve]  [rtp sender]
                                  |
                                  V
                          [clip writer]


Sample:
    v4l2src ! autoconvert ! tee name=splitter
        ! autovideosink name=displaysink async=0 splitter.
        ! valve name=videovalve drop=1 ! queue max-size-bytes=0 ! x264enc name=videoencoder ! tee name=videosplitter
                ! queue ! mpegtsmux name=videomux ! multifilesink name=videosink next-file=4 sync=0 async=0 location=/video/video videosplitter.
                ! queue ! rtph264pay ! udpsink name=rtpsink clients=127.0.0.1:5000 sync=0 videosplitter.
                ! valve name=clipinspect drop=1 ! queue ! mpegtsmux name=clipmux ! multifilesink name=clipsink next-file=4 post-messages=1 sync=0 async=0 location=/video/clip videosplitter.
        splitter.
        ! identity name=imagevalve drop-probability=1.0 ! jpegenc ! multifilesink name=imagesink post-messages=1 async=0 sync=0 location=/video/image splitter.
*/

QString MainWindow::buildPipeline()
{
    QSettings settings;

    // v4l2src device=/dev/video1 ! video/x-raw-yuv,format=(fourcc)YUY2,width=(int)720,height=(int)576 ! ffmpegcolorspace
    //
    QString pipe;

    auto deviceDef = settings.value("device").toString();
    auto formatDef = settings.value("format").toString();
    auto sizeDef   = settings.value("size").toSize();
    auto srcDef   = settings.value("src").toString();
    auto srcFixColor = settings.value("src-colorspace", false).toBool()? "! ffmpegcolorspace ": "";
    auto srcParams = settings.value("src-parameters").toString();

    if (!srcDef.isEmpty())
    {
        pipe.append(srcDef);
    }
    else
    {
        pipe.append(PLATFORM_SPECIFIC_SOURCE).append(" ").append(srcParams);
        if (!deviceDef.isEmpty())
        {
            pipe.append(" " PLATFORM_SPECIFIC_PROPERTY "=\"").append(deviceDef).append("\"");
        }
    }

    if (!formatDef.isEmpty())
    {
        pipe.append(" ! ").append(formatDef);
        if (!sizeDef.isEmpty())
        {
            pipe = pipe.append(",width=(int)%1,height=(int)%2").arg(sizeDef.width()).arg(sizeDef.height());
        }
    }

    pipe.append(srcFixColor);

    // v4l2src ... ! tee name=splitter ! ffmpegcolorspace ! ximagesink splitter.");
    //
    auto displaySinkDef = settings.value("display-sink",  "autovideosink name=displaysink async=0").toString();
    auto displayFixColor = settings.value(displaySinkDef + "-colorspace", false).toBool()? "ffmpegcolorspace ! ": "";
    auto displayParams = settings.value(displaySinkDef + "-parameters").toString();
    pipe.append(" ! tee name=splitter");
    if (!displaySinkDef.isEmpty())
    {
        pipe.append(" ! ").append(displayFixColor)
            .append(displaySinkDef).append(" ").append(displayParams).append(" splitter.");
    }

    // ... ! tee name=splitter ! ximagesink splitter. ! valve name=videovalve ! ffmpegcolorspace ! x264enc
    //           ! tee name=videosplitter
    //                ! queue ! mpegtsmux ! filesink videosplitter.  ! queue ! x264 videosplitter.
    //                ! queue ! rtph264pay ! udpsink videosplitter.
    //                ! identity name=clipinspect ! queue ! mpegtsmux ! filesink videosplitter.
    //           splitter.
    //
    auto outputPathDef      = settings.value("output-path",   "/video").toString();
    auto videoEncoderDef    = settings.value("video-encoder", "x264enc").toString();
    auto videoFixColor      = settings.value(videoEncoderDef + "-colorspace", false).toBool()? "ffmpegcolorspace ! ": "";
    auto videoEncoderParams = settings.value(videoEncoderDef + "-params").toString();
    auto videoMuxDef        = settings.value("video-muxer",     "mpegtsmux").toString();
    auto videoSinkDef       = settings.value("video-sink",    "multifilesink next-file=4 sync=0 async=0").toString();
    auto rtpSinkDef         = settings.value("rtp-sink",      "rtph264pay ! udpsink name=rtpsink clients=127.0.0.1:5000 sync=0").toString();
    auto clipSinkDef        = settings.value("clip-sink",     "multifilesink next-file=4 post-messages=1 sync=0 async=0").toString();
    auto enableVideo        = !videoSinkDef.isEmpty() && settings.value("enable-video").toBool();
    auto enableRtp          = !rtpSinkDef.isEmpty() && settings.value("enable-rtp").toBool();

    if (!videoEncoderDef.isEmpty() && (enableRtp || enableVideo || !clipSinkDef.isEmpty()))
    {
        pipe.append(" ! valve name=videovalve drop=1 ! queue max-size-bytes=0 ! ").append(videoFixColor)
                .append(videoEncoderDef).append(" name=videoencoder ").append(videoEncoderParams);
        if (enableRtp || enableVideo)
        {
            pipe.append(" ! tee name=videosplitter");
            if (enableVideo)
                pipe.append(" ! queue ! ").append(videoMuxDef).append(" name=videomux ! ")
                    .append(videoSinkDef).append(" name=videosink location=").append(outputPathDef).append("/video videosplitter.");
            if (enableRtp)
                pipe.append(" ! queue ! ").append(rtpSinkDef).append(" videosplitter.");
        }

        if (!clipSinkDef.isEmpty())
        {
            pipe.append(" ! identity name=clipinspect drop-probability=1.0 ! valve name=clipvalve drop=1 ! queue ! ").append(videoMuxDef).append(" name=clipmux ! ")
                .append(clipSinkDef).append(" name=clipsink location=").append(outputPathDef).append("/clip");
            if (enableRtp || enableVideo)
                pipe.append(" videosplitter.");
        }
        pipe.append(" splitter.");
    }

    // ... ! tee name=splitter ... splitter. ! identity name=imagevalve ! jpegenc ! multifilesink splitter.
    //
    auto imageEncoderDef = settings.value("image-encoder", "jpegenc").toString();
    auto imageEncoderFixColor = settings.value(imageEncoderDef + "-colorspace", false).toBool()?
                "ffmpegcolorspace ! ": "";
    auto imageEncoderParams = settings.value(imageEncoderDef + "-parameters").toString();
    auto imageSinkDef    = settings.value("image-sink", "multifilesink name=imagesink post-messages=1 async=0 sync=0").toString();
    if (!imageSinkDef.isEmpty())
    {
        pipe.append(" ! identity name=imagevalve drop-probability=1.0 ! ")
            .append(imageEncoderFixColor).append(imageEncoderDef).append(" ").append(imageEncoderParams).append(" ! ")
            .append(imageSinkDef).append(" location=").append(outputPathDef).append("/image splitter.");
    }

    return pipe;
}

QGst::PipelinePtr MainWindow::createPipeline()
{
    qCritical() << pipelineDef;

    QSettings settings;
    QGst::PipelinePtr pl;

    // Default values for all profiles
    //

    try
    {
        pl = QGst::Parse::launch(pipelineDef).dynamicCast<QGst::Pipeline>();
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
        {
            qCritical() << "Element displaysink not found";
        }

        clipInspect = pl->getElementByName("clipinspect");
        if (!clipInspect)
        {
            qCritical() << "Element clipinspect not found";
        }
        else
        {
            QGlib::connect(clipInspect, "handoff", this, &MainWindow::onClipFrame);
        }

        if (settings.value("enable-video").toBool())
        {
            videoSink  = pl->getElementByName("videosink");
            if (!videoSink)
            {
                qCritical() << "Element videosink not found";
            }
        }

        videoValve  = pl->getElementByName("videovalve");
        if (!videoValve)
        {
            qCritical() << "Element videovalve not found";
        }

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
        {
            qCritical() << "Element imagesink not found";
        }

        // To set correct bitrate we must examine default bitrate first
        //
        videoEncoder = pl->getElementByName("videoencoder");
        if (!videoEncoder)
        {
            qCritical() << "Element videoencoder not found";
        }

//        QGlib::connect(pl->getElementByName("test"), "handoff", this, &MainWindow::onTestHandoff);

        auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS((GstBin*)(QGst::Pipeline::CType*)pl, details, qApp->applicationName().toUtf8());

        // The pipeline will start once it reaches paused state without an error
        //
        pl->setState(QGst::StatePaused);
    }

    return pl;
}

void MainWindow::setElementProperty(const char* elmName, const char* prop, const QGlib::Value& value, QGst::State minimumState)
{
    auto elm = pipeline? pipeline->getElementByName(elmName): QGst::ElementPtr();
    if (!elm)
    {
        qDebug() << "Element " << elmName << " not found";
    }
    else
    {
        setElementProperty(elm, prop, value, minimumState);
    }
}

void MainWindow::setElementProperty(QGst::ElementPtr& elm, const char* prop, const QGlib::Value& value, QGst::State minimumState)
{
    if (elm)
    {
        QGst::State currentState = QGst::StateVoidPending;
        elm->getState(&currentState, nullptr, 1000000000L); // 1 sec
        if (currentState > minimumState)
        {
            elm->setState(minimumState);
            elm->getState(nullptr, nullptr, 1000000000L);
        }
        if (prop)
        {
            elm->setProperty(prop, value);
        }
        elm->setState(currentState);
        elm->getState(nullptr, nullptr, 1000000000L);
    }
}

// mpegtsmux => mpg, jpegenc => jpg, pngenc => png, oggmux => ogg, avimux => avi, matrosskamux => mat
//
static QString getExt(QString str)
{
    return QString(".").append(str.remove('e').left(3));
}

QString MainWindow::replace(QString str, int seqNo)
{
    const QString nn = seqNo >= 10? QString::number(seqNo): QString("0").append('0' + seqNo);
    const QDateTime ts = QDateTime::currentDateTime();

    return str
        .replace("%name%", patientName)
        .replace("%study%", studyName)
        .replace("%yyyy%", ts.toString("yyyy"))
        .replace("%yy%", ts.toString("yy"))
        .replace("%MM%", ts.toString("MM"))
        .replace("%MMM%", ts.toString("MMM"))
        .replace("%MMMM%", ts.toString("MMMM"))
        .replace("%dd%", ts.toString("dd"))
        .replace("%mm%", ts.toString("mm"))
        .replace("%nn%", nn)
        ;
}

void MainWindow::updatePipeline()
{
    QWaitCursor wait(this);
    QSettings settings;

    auto newPipelineDef = buildPipeline();
    if (newPipelineDef != pipelineDef)
    {
        qDebug() << "The pipeline has been changed, restarting";
        if (pipeline)
        {
            releasePipeline();
        }
        pipelineDef = newPipelineDef;
        pipeline = createPipeline();
    }

    setElementProperty("rtpsink", "clients", settings.value("rtp-clients").toString(), QGst::StateReady);

    if (videoEncoder)
    {
        auto videoEncBitrate = settings.value("bitrate", DEFAULT_VIDEOBITRATE).toInt();
        // To set correct bitrate we must examine default bitrate first
        //
        auto currentBitrate = videoEncoder->property("bitrate").toInt();
        if (currentBitrate > 200000)
        {
            // The codec uses bits per second instead of kbits per second
            //
            videoEncBitrate *= 1024;
        }
        setElementProperty(videoEncoder, "bitrate", videoEncBitrate);
    }

    outputPath.setPath(replace(settings.value("output-path", "/video").toString()
        .append(settings.value("folder-template", "/%yyyy%-%MM%/%dd%/%name%/").toString()), ++studyNo));

    if (!outputPath.mkpath("."))
    {
        QString msg = tr("Failed to create '%1'").arg(outputPath.absolutePath());
        qCritical() << msg;
        QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
        return;
    }

    if (videoSink)
    {
        QString videoExt = getExt(settings.value("video-muxer", "mpegtsmux").toString());
        QString videoFileName  = replace(settings.value("video-file", "video-%study%").toString(), studyNo).append(videoExt);

        // Video sink must be in null state to change the location
        //
        videoSink->setState(QGst::StateNull);
        videoSink->setProperty("location", outputPath.absoluteFilePath(videoFileName));
        videoSink->setState(QGst::StatePlaying);

        // Restart some elements
        //
        setElementProperty("videostamp", nullptr, nullptr, QGst::StateReady);
        setElementProperty("videomux", nullptr, nullptr, QGst::StateReady);
    }

    if (archiveWindow != nullptr)
    {
        archiveWindow->setRoot(settings.value("output-path", "/video").toString());
    }
}

void MainWindow::releasePipeline()
{
    pipeline->setState(QGst::StateNull);
    pipeline->getState(NULL, NULL, 10000000000L); // 10 sec

    displaySink.clear();
    imageValve.clear();
    imageSink.clear();
    clipInspect.clear();
    videoValve.clear();
    videoSink.clear();
    videoEncoder.clear();

    pipeline.clear();
    displayWidget->stopPipelineWatch();
}

void MainWindow::onClipFrame(const QGst::BufferPtr& buf)
{
    // Once we got an I-Frame, open second valve
    //
    if (0 == (buf->flags() & GST_BUFFER_FLAG_DELTA_UNIT))
    {
        btnRecord->setEnabled(true);

        // Restart some elements
        //
        setElementProperty("clipstamp", nullptr, nullptr, QGst::StateReady);
        setElementProperty("clipmux", nullptr, nullptr, QGst::StateReady);

        setElementProperty("clipvalve", "drop", false);
        setElementProperty("displayoverlay", "text", "R");

        if (!clipPreviewFileName.isEmpty())
        {
            // Once an image will be ready, the valve will be turned off again.
            //
            btnSnapshot->setEnabled(false);

            // Take a picture for thumbnail
            //
            setElementProperty(imageSink, "location", clipPreviewFileName, QGst::StateReady);

            // Turn the valve on for a while.
            //
            imageValve->setProperty("drop-probability", 0.0);
        }
    }
}

void MainWindow::onImageReady(const QGst::BufferPtr& buf)
{
    qDebug() << "imageValve handoff " << buf->size() << " " << buf->timeStamp() << " " << buf->flags();
    imageValve->setProperty("drop-probability", 1.0);
}

void MainWindow::errorGlib(const QGlib::ObjectPtr& obj, const QGlib::Error& ex)
{
    const QString msg = obj?
        QString().append(obj->property("name").toString()).append(" ").append(ex.message()):
        ex.message();
    qCritical() << msg;
    QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
}

void MainWindow::onBusMessage(const QGst::MessagePtr& message)
{
    //qDebug() << message->typeName() << " " << message->source()->property("name").toString();

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
        QString fileName = s->value("filename").toString();
        QPixmap pm;

        auto lastBuffer = message->source()->property("last-buffer").get<QGst::BufferPtr>();
        bool ok = lastBuffer && pm.loadFromData(lastBuffer->data(), lastBuffer->size());

        // If we can not load from the buffer, try to load from the file
        //
        if (ok || pm.load(fileName))
        {
            if (clipPreviewFileName == fileName)
            {
                // Got a snapshot for a clip file. Add a fency overlay to it
                //
                QPixmap pmOverlay(":/buttons/film");
                QPainter painter(&pm);
                painter.setOpacity(0.75);
                painter.drawPixmap(pm.rect(), pmOverlay);
                clipPreviewFileName.clear();
            }

            QListWidgetItem* item = new QListWidgetItem(QIcon(pm), QFileInfo(fileName).baseName(), imageList);
            item->setToolTip(fileName);
            imageList->setItemSelected(item, true);
            imageList->scrollToItem(item);
        }
        else
        {
            QListWidgetItem* item = new QListWidgetItem(QIcon(":/buttons/stop"), tr("(error)"), imageList);
            item->setToolTip(tr("Failed to load image %1").arg(fileName));
        }

        btnSnapshot->setEnabled(running);
        return;
    }

    if (s->name() == "prepare-xwindow-id" || s->name() == "prepare-window-handle")
    {
        // At this time the video output finally has a sink, so set it up now
        //
        QGst::ElementPtr sink = displayWidget->videoSink();
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
        imageNo = clipNo = 0;
        imageList->clear();

#ifdef WITH_DICOM

        if (worklist == nullptr)
        {
            worklist = new Worklist();
        }
        StartStudyDialog dlg(worklist, this);
        if (QDialog::Rejected == dlg.exec())
        {
            // Cancelled
            //
            return;
        }

        auto patientDs = worklist->getPatientDS();
        if (patientDs == nullptr)
        {
            QMessageBox::critical(this, windowTitle(), tr("No patient selected"));
            return;
        }

        DcmClient client(UID_ModalityPerformedProcedureStepSOPClass);
        pendingSOPInstanceUID = client.nCreateRQ(patientDs);
        delete patientDs;

        if (pendingSOPInstanceUID.isNull())
        {
            QMessageBox::critical(this, windowTitle(), client.lastError());
            return;
        }
#else
        PatientDialog dlg(this);
        if (!dlg.exec())
        {
            // User cancelled
            //
            return;
        }
        patientName = dlg.patientName();
        studyName   = dlg.studyName();
#endif
        QWaitCursor wait(this);
        updatePipeline();
        // After updatePipeline the outputPath is usable
        //
        dlg.savePatientFile(outputPath.absoluteFilePath(".patient"));
        running = true;
    }
    else
    {
        clipPreviewFileName.clear();
        setElementProperty(clipInspect, "drop-probability", 1.0);
        setElementProperty("clipvalve", "drop", true);

        int userChoice;
#ifdef WITH_DICOM
        userChoice = QMessageBox::question(this, windowTitle(),
           tr("Send study results to the server?"), tr("Continue the study"), tr ("Don't sent"), tr("Send"), 2, 0);

        if (userChoice == 0)
        {
            // Continue the study
            //
            return;
        }
#else
        userChoice = QMessageBox::question(this, windowTitle(),
           tr("End the study?"), QMessageBox::Yes | QMessageBox::Default, QMessageBox::No);

        if (userChoice == QMessageBox::No)
        {
            // Don't end the study
            //
            return;
        }
#endif

        QWaitCursor wait(this);
        running = recording = false;
        updateRecordButton();

#ifdef WITH_DICOM
        auto patientDs = worklist->getPatientDS();

        QString   seriesUID;
        if (!pendingSOPInstanceUID.isEmpty())
        {
            DcmClient client(UID_ModalityPerformedProcedureStepSOPClass);
            seriesUID = client.nSetRQ(patientDs, pendingSOPInstanceUID);
            if (seriesUID.isEmpty())
            {
                QMessageBox::critical(this, windowTitle(), client.lastError());
            }
        }

        if (userChoice == 2)
        {
            sendToServer(patientDs, seriesUID);
        }

        pendingSOPInstanceUID.clear();
        delete patientDs;
#endif
    }

    setElementProperty(videoValve, "drop", !running);

    updateStartButton();
    updateRecordAll();

    //imageOut->clear();
    displayWidget->update();
}

void MainWindow::onSnapshotClick()
{
    QSettings settings;
    QString imageExt = getExt(settings.value("image-encoder", "jpegenc").toString());
    QString imageFileName = replace(settings.value("image-file", "image-%study%-%nn%").toString(), ++imageNo).append(imageExt);

    setElementProperty(imageSink, "location", outputPath.absoluteFilePath(imageFileName), QGst::StateReady);

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
        // Until the real clip recording starts, we should disable this button
        //
        btnRecord->setEnabled(false);

        QSettings settings;

        // Manually increment video clip file name
        //
        QString videoExt = getExt(settings.value("video-muxer", "mpegtsmux").toString());
        QString imageExt = getExt(settings.value("image-encoder", "jpegenc").toString());
        QString clipFileName = replace(settings.value("clip-file",  "clip-%study%-%nn%").toString(), ++clipNo).append(videoExt);

        setElementProperty("clipsink", "location", outputPath.absoluteFilePath(clipFileName), QGst::StateNull);
        clipPreviewFileName = outputPath.absoluteFilePath(clipFileName.append(imageExt));
    }
    else
    {
        clipInspect->sendEvent(QGst::FlushStartEvent::create());
        clipInspect->sendEvent(QGst::FlushStopEvent::create());
        setElementProperty("clipvalve", "drop", true);
        setElementProperty("displayoverlay", "text", "");
    }

    clipInspect->setProperty("drop-probability", recording? 0.0: 1.0);
}

void MainWindow::updateStartButton()
{
    QIcon icon(running? ":/buttons/stop": ":/buttons/start");
    QString strOnOff(running? tr("End &study"): tr("Start &study"));
    btnStart->setIcon(icon);
    btnStart->setText(strOnOff);

    btnRecord->setEnabled(running && pipeline->getElementByName("clipsink"));
    btnSnapshot->setEnabled(running && pipeline->getElementByName("imagesink"));
    actionSettings->setEnabled(!running);
    //displayWidget->setVisible(running && displaySink);
#ifdef WITH_DICOM
    if (worklist)
    {
        worklist->setDisabled(running);
    }
#endif
}

void MainWindow::updateRecordButton()
{
    QIcon icon(recording? ":/buttons/pause": ":/buttons/record");
    QString strOnOff(recording? tr("Paus&e"): tr("R&ecord"));
    btnRecord->setIcon(icon);
    btnRecord->setText(strOnOff);
}

void MainWindow::updateRecordAll()
{
    QString strOnOff(videoSink? tr("on"): tr("off"));
    const char* icon = videoSink? ":/buttons/record_on": ":/buttons/record_off";

    lblRecordAll->setEnabled(running);
    lblRecordAll->setToolTip(tr("Recording of entire study is %1").arg(strOnOff));
    lblRecordAll->setPixmap(QIcon(icon).pixmap(lblRecordAll->size()));
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
    updatePipeline();
}

void MainWindow::onShowAboutClick()
{
    AboutDialog(this).exec();
}

void MainWindow::onShowArchiveClick()
{
    if (archiveWindow == nullptr)
    {
        archiveWindow = new ArchiveWindow();
        archiveWindow->setRoot(QSettings().value("output-path", "/video").toString());
    }
    archiveWindow->setPath(outputPath.absolutePath());
    archiveWindow->show();
    archiveWindow->activateWindow();
}

void MainWindow::onShowSettingsClick()
{
    Settings dlg(this);
    connect(&dlg, SIGNAL(apply()), this, SLOT(updatePipeline()));
    if (dlg.exec())
    {
        updatePipeline();
    }
}

#ifdef WITH_DICOM
void MainWindow::onShowWorkListClick()
{
    if (worklist == nullptr)
    {
        worklist = new Worklist();
    }
    worklist->show();
    worklist->activateWindow();
}

void MainWindow::sendToServer(DcmDataset* patientDs, const QString& seriesUID)
{
    QWaitCursor wait(this);
    QProgressDialog pdlg(this);
    outputPath.setFilter(QDir::Files | QDir::Readable);
    pdlg.setRange(0, outputPath.count());
    DcmClient client(UID_MultiframeTrueColorSecondaryCaptureImageStorage);

    // Only single series for now
    //
    int seriesNo = 1;

    for (uint i = 0; !pdlg.wasCanceled() && i < outputPath.count(); ++i)
    {
        pdlg.setValue(i);
        pdlg.setLabelText(tr("Storing '%1'").arg(outputPath[i]));
        qApp->processEvents();

        const QString& file = outputPath.absoluteFilePath(outputPath[i]);
        if (!client.sendToServer(patientDs, seriesUID, seriesNo, file, i))
        {
            QMessageBox::critical(this, windowTitle(), client.lastError());
        }
    }
}
#endif
