#include "videoeditor.h"
#include "product.h"
#include "defaults.h"
#include "qwaitcursor.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QDebug>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QTemporaryFile>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QxtSpanSlider>

#include <QGlib/Connect>
#include <QGlib/Error>
#include <QGst/Bus>
#include <QGst/ElementFactory>
#include <QGst/Event>
#include <QGst/Parse>
#include <QGst/Query>
#include <QGst/Ui/VideoWidget>
#include <gst/gstdebugutils.h>

#define SLIDER_SCALE 20000L

static QSize videoSize(352, 258);

static void DamnQtMadeMeDoTheSunsetByHands(QToolBar* bar)
{
    foreach (auto action, bar->actions())
    {
        auto shortcut = action->shortcut();
        if (shortcut.isEmpty())
        {
            continue;
        }
        action->setToolTip(action->text() + " (" + shortcut.toString(QKeySequence::NativeText) + ")");
    }
}

VideoEditor::VideoEditor(QWidget *parent)
    : QWidget(parent)
    , duration(0LL)
{
    auto layoutMain = new QVBoxLayout;

    auto barEditor = new QToolBar(tr("Video editor"));
    barEditor->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    auto actionBack = barEditor->addAction(QIcon(":buttons/back"), tr("Exit"), this, SLOT(close()));
    actionBack->setShortcut(QKeySequence::Close);

    auto spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    barEditor->addWidget(spacer);

    auto actionSave = barEditor->addAction(QIcon(":buttons/back"), tr("Save"), this, SLOT(onSaveClick()));
    actionSave->setShortcut(QKeySequence::Save);
    auto actionSaveAs = barEditor->addAction(QIcon(":buttons/back"), tr("Save as").append(0x2026), this, SLOT(onSaveAsClick()));
    actionSaveAs->setShortcut(QKeySequence::SaveAs);

#ifndef WITH_TOUCH
    layoutMain->addWidget(barEditor);
#endif

    videoWidget = new QGst::Ui::VideoWidget;
    layoutMain->addWidget(videoWidget);
    videoWidget->setMinimumSize(videoSize);
    videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto barMediaControls = new QToolBar(tr("Media"));
    barMediaControls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lblPos = new QLabel;
    lblPos->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lblPos->setAlignment(Qt::AlignLeft);
    barMediaControls->addWidget(lblPos);
    auto actionSeekBack = barMediaControls->addAction(QIcon(":buttons/rewind"), tr("Rewing"), this, SLOT(onSeekClick()));
    actionSeekBack->setData(-40000000);
    actionSeekBack->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Left));
    auto actionPlayAll = barMediaControls->addAction(QIcon(":buttons/record"), tr("Play all"), this, SLOT(onPlayPauseClick()));
    actionPlayAll->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Space));
    auto actionPlayCrop = barMediaControls->addAction(QIcon(":buttons/record"), tr("Play crop"), this, SLOT(onPlayPauseClick()));
    actionPlayCrop->setShortcut(QKeySequence(Qt::Key_Space));
    auto actionSeekFwd = barMediaControls->addAction(QIcon(":buttons/forward"),  tr("Forward"), this, SLOT(onSeekClick()));
    actionSeekFwd->setData(+40000000);
    actionSeekFwd->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Right));
    lblRange = new QLabel;
    lblRange->setAlignment(Qt::AlignRight);
    lblRange->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    barMediaControls->addWidget(lblRange);
    layoutMain->addWidget(barMediaControls, 0, Qt::AlignJustify);

    sliderPos = new QSlider(Qt::Horizontal);
    sliderPos->setTickInterval(SLIDER_SCALE / 100);
    sliderPos->setMaximum(SLIDER_SCALE);
    connect(sliderPos, SIGNAL(sliderMoved(int)), this, SLOT(setPosition(int)));
    layoutMain->addWidget(sliderPos);
    sliderRange = new QxtSpanSlider(Qt::Horizontal);
    sliderRange->setTickPosition(QSlider::TicksAbove);
    sliderRange->setTickInterval(SLIDER_SCALE / 100);
    sliderRange->setMaximum(SLIDER_SCALE);
    sliderRange->setUpperValue(SLIDER_SCALE);
    connect(sliderRange, SIGNAL(lowerPositionChanged(int)), this, SLOT(setPosition(int)));
    connect(sliderRange, SIGNAL(upperPositionChanged(int)), this, SLOT(setPosition(int)));
    layoutMain->addWidget(sliderRange);

#ifdef WITH_TOUCH
    layoutMain->addWidget(barEditor);
#endif
    setLayout(layoutMain);

    QSettings settings;
    restoreGeometry(settings.value("video-editor-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("video-editor-state").toInt());

    DamnQtMadeMeDoTheSunsetByHands(barEditor);
    DamnQtMadeMeDoTheSunsetByHands(barMediaControls);


    // This timer is used to tell the ui to change its position slider & label
    // every 100 ms, but only when the pipeline is playing
    //
    positionTimer = new QTimer(this);
    connect(positionTimer, SIGNAL(timeout()), this, SLOT(onPositionChanged()));
}

VideoEditor::~VideoEditor()
{
    if (pipeline)
    {
        videoWidget->stopPipelineWatch();
        pipeline->setState(QGst::StateNull);
    }
}

void VideoEditor::closeEvent(QCloseEvent *evt)
{
    QSettings settings;
    settings.setValue("video-editor-geometry", saveGeometry());
    settings.setValue("video-editor-state", (int)windowState() & ~Qt::WindowMinimized);
    QWidget::closeEvent(evt);
}

void VideoEditor::loadFile(const QString& filePath)
{
    qDebug() << filePath;

    sliderPos->setValue(0);
    duration = 0LL;

    if (!pipeline)
    {
        pipeline = QGst::ElementFactory::make("playbin2").dynamicCast<QGst::Pipeline>();
        if (pipeline) {
            //let the video widget watch the pipeline for new video sinks
            videoWidget->watchPipeline(pipeline);

            //watch the bus for messages
            QGst::BusPtr bus = pipeline->bus();
            bus->addSignalWatch();
            QGlib::connect(bus, "message", this, &VideoEditor::onBusMessage);
        }
        else
        {
            QMessageBox::critical(this, windowTitle(), tr("Failed to create the pipeline"), QMessageBox::Ok);
        }
    }

    if (pipeline)
    {
        pipeline->setProperty("uri", QUrl::fromLocalFile(filePath).toEncoded());
        auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(pipeline.staticCast<QGst::Bin>(), details, qApp->applicationName().append(".video-edit-preview").toUtf8());
        pipeline->setState(QGst::StatePaused);
        setWindowTitle(tr("Video editor - %1").arg(filePath));
        this->filePath = filePath;
    }
}

void VideoEditor::onBusMessage(const QGst::MessagePtr& message)
{
    //qDebug() << message->typeName() << " " << message->source()->property("name").toString();
    switch (message->type())
    {
    case QGst::MessageStateChanged:
        onStateChange(message.staticCast<QGst::StateChangedMessage>());
        break;
    case QGst::MessageAsyncDone:
        {
            // Here we query the pipeline about the content's duration
            // and we request that the result is returned in time format
            //
            QGst::DurationQueryPtr queryLen = QGst::DurationQuery::create(QGst::FormatTime);
            duration = pipeline && pipeline->query(queryLen)? queryLen->duration(): 0LL;
        }
        break;
    case QGst::MessageEos: //End of stream. We reached the end of the file.
        if (pipeline)
        {
            pipeline->setState(QGst::StateNull);
        }
        break;
    case QGst::MessageElement:
        {
            const QGst::StructurePtr s = message->internalStructure();
            if (!s || !message->source())
            {
                qDebug() << "Got empty QGst::MessageElement";
            }
            else if (s->name() == "prepare-xwindow-id" || s->name() == "prepare-window-handle")
            {
                // At this time the video output finally has a sink, so set it up now
                //
                message->source()->setProperty("force-aspect-ratio", true);
                videoWidget->update();
            }
        }
        break;
    case QGst::MessageError:
        {
            auto ex = message.staticCast<QGst::ErrorMessage>()->error();
            auto obj = message->source();
            QString msg;
            if (obj)
            {
                msg.append(obj->property("name").toString()).append(' ');
            }
            msg.append(ex.message());

            qCritical() << msg;
#ifndef Q_WS_WIN
            // Showing a message box under Microsoft (R) Windows (TM) breaks everything,
            // since it becomes the active one and the video output goes here.
            // So we can no more then hide this error from the user.
            //
            QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
#endif
        }
      break;
    default:
        break;
    }
}

void VideoEditor::onStateChange(const QGst::StateChangedMessagePtr& message)
{
    if (message->source() == pipeline)
    {
        if (message->newState() == QGst::StatePlaying)
        {
            positionTimer->start(20);
        }
        else if (message->oldState() == QGst::StatePlaying)
        {
            positionTimer->stop();
        }
    }
}

void VideoEditor::onPositionChanged()
{
    if (pipeline)
    {
        //here we query the pipeline about its position
        //and we request that the result is returned in time format
        QGst::PositionQueryPtr queryPos = QGst::PositionQuery::create(QGst::FormatTime);
        pipeline->query(queryPos);
        lblPos->setText(QGst::ClockTime(queryPos->position()).toTime().toString("hh:mm:ss.zzz")
            .append("\n").append(QGst::ClockTime(duration).toTime().toString("hh:mm:ss.zzz")));

        if (duration)
        {
            sliderPos->setValue(queryPos->position() * SLIDER_SCALE / duration);
        }
    }
}

void VideoEditor::setPosition(int position)
{
    if (pipeline)
    {
        //here we query the pipeline about the content's duration
        //and we request that the result is returned in time format
        QGst::DurationQueryPtr queryLen = QGst::DurationQuery::create(QGst::FormatTime);
        pipeline->query(queryLen);

        if (queryLen->duration())
        {
            QGst::SeekEventPtr evt = QGst::SeekEvent::create(
                1.0, QGst::FormatTime, QGst::SeekFlagFlush,
                QGst::SeekTypeSet, queryLen->duration() * position / SLIDER_SCALE,
                QGst::SeekTypeNone, QGst::ClockTime::None
            );

            pipeline->sendEvent(evt);
        }
    }
}

void VideoEditor::onSaveClick()
{
    QTemporaryFile tmpFile;
    exportVideo(&tmpFile);
    QFile::remove(filePath); // Make Microsoft (R) Windows (TM) happy
    QFile::rename(tmpFile.fileName(), filePath);
    tmpFile.setAutoRemove(false);
}

void VideoEditor::onSaveAsClick()
{
    QWaitCursor wait(this);
    QFileDialog dlg(this);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setFileMode(QFileDialog::AnyFile);
    QStringList filters = dlg.nameFilters();
    filters.insert(0, QString("*.").append(QFileInfo(filePath).suffix()));
    dlg.setNameFilters(filters);
    if (dlg.exec())
    {
        if (filePath == dlg.selectedFiles().first())
        {
            onSaveClick();
        }
        else
        {
            QFile file(dlg.selectedFiles().first());
            exportVideo(&file);
        }
    }
}

void VideoEditor::exportVideo(QFile* outFile)
{
    QSettings settings;
    auto encoder         = settings.value("video-encoder", DEFAULT_VIDEO_ENCODER).toString();
    auto fixColor        = settings.value(encoder + "-colorspace").toBool()? "ffmpegcolorspace ! ": "";
    auto encoderParams   = settings.value(encoder + "-parameters").toString();
    auto muxer           = settings.value("video-muxer", DEFAULT_VIDEO_MUXER).toString();
    auto videoEncBitrate = settings.value("bitrate", DEFAULT_VIDEOBITRATE).toInt();

    if (!outFile->open(QIODevice::WriteOnly))
    {
        // TODO
        return;
    }
    qDebug() << outFile->fileName();
    qint64 start = duration * sliderRange->lowerValue() / SLIDER_SCALE;
    qint64 len   = duration * sliderRange->upperValue() / SLIDER_SCALE - start;

    try
    {
        auto pipeDef = QString("gnlfilesource location=\"%1\" start=%2 duration=%3 media-start=0 media-duration=%3 !"
            " %4 %5 name=videoencoder %6 ! %7 ! fdsink fd=\"%8\"")
            .arg(filePath).arg(start).arg(len).arg(fixColor, encoder, encoderParams, muxer).arg(outFile->handle());
        qDebug() << pipeDef;
        auto pipelineExport = QGst::Parse::launch(pipeDef).dynamicCast<QGst::Pipeline>();

        auto videoEncoder = pipelineExport->getElementByName("videoencoder");
        if (videoEncoder)
        {
            // To set correct bitrate we must examine default bitrate first
            //
            auto currentBitrate = videoEncoder->property("bitrate").toInt();
            if (currentBitrate > 200000)
            {
                // The codec uses bits per second instead of kbits per second
                //
                videoEncBitrate *= 1024;
            }
            videoEncoder->setProperty("bitrate", videoEncBitrate);
        }

        //QGlib::connect(pipelineExport->bus(), "message", this, &ArchiveWindow::onBusMessage);
        //pipelineExport->bus()->addSignalWatch();
        auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(pipeline.staticCast<QGst::Bin>(), details, qApp->applicationName().append(".video-edit-export").toUtf8());

        pipelineExport->setState(QGst::StatePlaying);
        pipelineExport->getState(nullptr, nullptr, GST_SECOND * 10); // 10 sec
        QMessageBox::information(this, "Test", windowTitle(), QMessageBox::Ok);
        pipelineExport->setState(QGst::StateNull);
        pipelineExport->getState(nullptr, nullptr, GST_SECOND * 10); // 10 sec
    }
    catch (QGlib::Error ex)
    {
        const QString msg = ex.message();
        qCritical() << msg;
        QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
    }
}

void VideoEditor::onSeekClick()
{

}

void VideoEditor::onPlayPauseClick()
{

}
