#include "mainwindow.h"
#include "qwaitcursor.h"
#include "settings.h"

#ifdef WITH_DICOM
#include "worklist.h"
#include "startstudydialog.h"
#include "dcmclient.h"

#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmdata/dcdatset.h>
#else
#include "patientdialog.h"
#endif

#include <QApplication>
#include <QResizeEvent>
#include <QFrame>
#include <QBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>

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
    QWidget(parent),
#ifdef WITH_DICOM
    worklist(nullptr),
#endif
    running(false),
    recording(false)
{
    QSettings settings;

    QToolBar* bar = new QToolBar(tr("main"));

    btnStart = new QToolButton();
    btnStart->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnStart->setFocusPolicy(Qt::NoFocus);
    connect(btnStart, SIGNAL(clicked()), this, SLOT(onStartClick()));
    bar->addWidget(btnStart);

    btnSnapshot = new QToolButton();
    btnSnapshot->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnSnapshot->setFocusPolicy(Qt::NoFocus);
    btnSnapshot->setIcon(QIcon(":/buttons/camera"));
    btnSnapshot->setText(tr("&Take snapshot"));
    connect(btnSnapshot, SIGNAL(clicked()), this, SLOT(onSnapshotClick()));
    bar->addWidget(btnSnapshot);

    btnRecord = new QToolButton();
    btnRecord->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnRecord->setFocusPolicy(Qt::NoFocus);
    connect(btnRecord, SIGNAL(clicked()), this, SLOT(onRecordClick()));
    bar->addWidget(btnRecord);

    QWidget* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    bar->addWidget(spacer);

    lblRecordAll = new QLabel;
    lblRecordAll->setMaximumSize(bar->iconSize());
    bar->addWidget(lblRecordAll);

#ifdef WITH_DICOM
    QAction* actionWorklist = bar->addAction(QIcon(":/buttons/show_worklist"), nullptr, this, SLOT(onShowWorkListClick()));
    actionWorklist->setToolTip(tr("Show &work list"));
#endif

    QAction* actionArchive = bar->addAction(QIcon(":/buttons/folder"), nullptr, this, SLOT(onShowArchiveClick()));
    actionArchive->setToolTip(tr("Show studies archive"));
    QAction* actionSettings = bar->addAction(QIcon(":/buttons/settings"), nullptr, this, SLOT(onShowSettingsClick()));
    actionSettings->setToolTip(tr("Edit settings"));
    QAction* actionAbout = bar->addAction(QIcon(":/buttons/about"), nullptr, this, SLOT(onShowAboutClick()));
    actionAbout->setToolTip(tr("About berillyum"));

    displayWidget = new QGst::Ui::VideoWidget();
    displayWidget->setMinimumSize(720, 576);

    imageList = new QListWidget();
    imageList->setViewMode(QListView::IconMode);
    imageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    imageList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    imageList->setMinimumHeight(128);
    imageList->setMaximumHeight(128);
    imageList->setIconSize(QSize(128,128));
    imageList->setMovement(QListView::Static);
    imageList->setWrapping(false);

    QVBoxLayout* layoutMain = new QVBoxLayout();
    layoutMain->setMenuBar(createMenu());
    layoutMain->addWidget(bar);
    layoutMain->addWidget(displayWidget);
    layoutMain->addWidget(imageList);

    setLayout(layoutMain);

    updateStartButton();
    updateRecordButton();
    updateRecordAll();

    restartPipeline();
}

MainWindow::~MainWindow()
{
    if (pipeline)
    {
        releasePipeline();
    }

#ifdef WITH_DICOM
    delete worklist;
    worklist = nullptr;
#endif
}

QMenuBar* MainWindow::createMenu()
{
    auto mnuBar = new QMenuBar();
    auto mnu    = new QMenu(tr("&Menu"));

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
  [image valve]       |     [video valve]
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
*/


QGst::PipelinePtr MainWindow::createPipeline()
{
    QSettings settings;
    QGst::PipelinePtr pl;

    // Default values for all profiles
    //

    bool enableVideo = settings.value("enable-video").toBool();
    bool enableRtp   = settings.value("enable-rtp").toBool();

    QString outputPathFormat  = settings.value("output-path", "'/video/'yyyy-MM-dd/HH-mm'").toString();
    QString videoFileName     = settings.value("video-file", "video.mpg").toString();
    QString clipFileName      = settings.value("clip-file",  "clip%03d.mpg").toString();
    QString imageFileName     = settings.value("image-file", "image%03d.jpg").toString();
    QString srcDef            = settings.value("src", "autovideosrc").toString();
    QString displaySinkDef    = settings.value("display-sink",  "autovideosink name=displaysink async=0").toString();
    QString videoEncoderDef   = settings.value("video-encoder", "x264enc").toString();
    QString videoMuxDef       = settings.value("video-mux",     "mpegtsmux").toString();
    QString videoSinkDef      = settings.value("video-sink",    "filesink name=videosink sync=0 async=0").toString();
    QString rtpSinkDef        = settings.value("rtp-sink",      "rtph264pay ! udpsink name=rtpsink clients=127.0.0.1:5000 sync=0").toString();
    QString clipSinkDef       = settings.value("clip-sink",     "multifilesink name=clipsink next-file=4 sync=0 async=0").toString();
    QString imageEncoderDef   = settings.value("image-encoder", "jpegenc").toString();
    QString imageSinkDef      = settings.value("image-sink",    "multifilesink name=imagesink post-messages=1 async=0 sync=0").toString();

    int videoEncoderBitrate = settings.value("bitrate", 4000).toInt();
    QString rtpClients = enableRtp? settings.value("rtp-clients").toString(): QString();

    const QString& outputPathStr = QDateTime::currentDateTime().toString(outputPathFormat);
    outputPath = QDir(outputPathStr);
    if (!outputPath.mkpath("."))
    {
        QString msg = tr("Failed to create '%1'").arg(outputPathStr);
        qCritical() << msg;
        QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
        return pl;
    }

    if (videoSinkDef.isEmpty())
    {
        enableVideo = false;
    }

    if (rtpSinkDef.isEmpty())
    {
        enableRtp = false;
    }

    QString pipe = QString().append(srcDef).append(" ! autoconvert ! tee name=splitter");
    if (!displaySinkDef.isEmpty())
    {
        pipe.append(" ! ").append(displaySinkDef).append(" splitter.");
    }

    if (!videoEncoderDef.isEmpty() && (enableRtp || enableVideo || !clipSinkDef.isEmpty()))
    {
        pipe.append(" ! valve name=videovalve ! queue max-size-bytes=0 ! ").append(videoEncoderDef).append(" name=videoencoder");
        if (enableRtp || enableVideo)
        {
            pipe.append(" ! tee name=videosplitter");
            if (enableVideo)
                pipe.append(" ! queue ! ").append(videoMuxDef).append(" name=videomux ! ").append(videoSinkDef).append(" videosplitter.");
            if (enableRtp)
                pipe.append(" ! queue ! ").append(rtpSinkDef).append(" videosplitter.");
        }

        if (!clipSinkDef.isEmpty())
        {
            pipe.append(" ! valve name=clipvalve ! queue ! ").append(videoMuxDef).append(" name=clipmux ! ").append(clipSinkDef);
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
        {
            qCritical() << "Element displaysink not found";
        }

        clipValve = pl->getElementByName("clipvalve");
        if (!clipValve)
        {
            qCritical() << "Element clipvalve not found";
        }

        if (enableVideo)
        {
            videoSink  = pl->getElementByName("videosink");
            if (!videoSink)
            {
                qCritical() << "Element videosink not found";
            }
            else
            {
                videoSink->setProperty("location", outputPath.absoluteFilePath(videoFileName));
            }
        }

        clipSink  = pl->getElementByName("clipsink");
        if (!clipSink)
        {
            qCritical() << "Element clipsink not found";
        }
        else
        {
            clipSink->setProperty("location", outputPath.absoluteFilePath(clipFileName));
            clipSink->setProperty("index", 0);
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
        else
        {
            imageSink->setProperty("location", outputPath.absoluteFilePath(imageFileName));
            imageSink->setProperty("index", 0);
        }

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
                int defaultBitrate = videoEncoder->property("bitrate").toInt();
                if (defaultBitrate > 200000)
                {
                    // code uses bits per second instead of kbits
                    videoEncoderBitrate *= 1024;
                }
                videoEncoder->setProperty("bitrate", videoEncoderBitrate);
            }
        }

//        QGlib::connect(pl->getElementByName("test"), "handoff", this, &MainWindow::onTestHandoff);

        auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS((GstBin*)(QGst::Pipeline::CType*)pl, details, qApp->applicationName().toUtf8());
    }

    return pl;
}

void MainWindow::restartPipeline()
{
    if (pipeline)
    {
        releasePipeline();
    }

    pipeline = createPipeline();
    if (pipeline)
    {
        if (videoValve)
        {
            videoValve->setProperty("drop", !running);
        }

        if (imageValve)
        {
            imageValve->setProperty("drop-probability", 1.0);
        }

        pipeline->setState(QGst::StatePaused);
    }
}

void MainWindow::releasePipeline()
{
    pipeline->setState(QGst::StateNull);
    pipeline->getState(NULL, NULL, 10000000000L); // 10 sec

    displaySink.clear();
    imageValve.clear();
    imageSink.clear();
    clipValve.clear();
    clipSink.clear();
    videoValve.clear();
    videoSink.clear();
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
    const QString msg = obj?
        QString().append(obj->property("name").toString()).append(" ").append(ex.message()):
        ex.message();
    qCritical() << msg;
    QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
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
    elm->getState(NULL, NULL, 1000000000L); // 1 sec
    elm->setState(QGst::StatePlaying);
    elm->getState(NULL, NULL, 1000000000L); // 1 sec
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
        const int index = s->value("index").toInt();
        QPixmap pm;

        auto lastBuffer = message->source()->property("last-buffer").get<QGst::BufferPtr>();
        bool ok = lastBuffer && pm.loadFromData(lastBuffer->data(), lastBuffer->size());

        // If we can not load from the buffer, try to load from the file
        //
        ok = ok || pm.load(fileName);

        if (ok)
        {
            QListWidgetItem* item = new QListWidgetItem(QIcon(pm), QString::number(index), imageList);
            item->setToolTip(fileName);
            imageList->setItemSelected(item, true);
            imageList->scrollToItem(item);
        }
        else
        {
            QListWidgetItem* item = new QListWidgetItem(tr("(error)"), imageList);
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
#endif
        QWaitCursor wait(this);
        pipeline = createPipeline();
        if (pipeline)
        {
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
        releasePipeline();

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

    updateStartButton();
    updateRecordAll();

    //imageOut->clear();
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

void MainWindow::updateStartButton()
{
    QIcon icon(running? ":/buttons/stop": ":/buttons/start");
    QString strOnOff(running? tr("End &study"): tr("Start &study"));
    btnStart->setIcon(icon);
    btnStart->setText(strOnOff);

    btnRecord->setEnabled(running && clipSink);
    btnSnapshot->setEnabled(running && imageSink);
//    displayWidget->setVisible(running && displaySink);
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
    QString strOnOff(recording? tr("Pause"): tr("Record"));
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
}

void MainWindow::onShowAboutClick()
{
    QMessageBox::information(this, windowTitle(), "Not yet");
}

void MainWindow::onShowArchiveClick()
{
    QMessageBox::information(this, windowTitle(), "Not yet");
}

void MainWindow::onShowSettingsClick()
{
    Settings dlg(this);
    if (dlg.exec())
    {
        restartPipeline();
    }
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
