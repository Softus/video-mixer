/*
 * Copyright (C) 2013-2014 Irkutsk Diagnostic Center.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "videoeditor.h"
#include "videoencodingprogressdialog.h"
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
#include <QGst/Buffer>
#include <QGst/Bus>
#include <QGst/ElementFactory>
#include <QGst/Event>
#include <QGst/Fourcc>
#include <QGst/Pad>
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

VideoEditor::VideoEditor(const QString& filePath, QWidget *parent)
    : QWidget(parent)
    , filePath(filePath)
    , duration(0LL)
    , frameDuration(40000000LL)
{
    auto layoutMain = new QVBoxLayout;

    auto barEditor = new QToolBar(tr("Video editor"));
    barEditor->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    auto actionBack = barEditor->addAction(QIcon(":buttons/back"), tr("Exit"), this, SLOT(close()));
    actionBack->setShortcut(QKeySequence::Close);

    auto spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    barEditor->addWidget(spacer);

    auto actionSave = barEditor->addAction(QIcon(":buttons/file_save"), tr("Save"), this, SLOT(onSaveClick()));
    actionSave->setShortcut(QKeySequence::Save);
    auto actionSaveAs = barEditor->addAction(QIcon(":buttons/file_save_as"), tr("Save as").append(0x2026), this, SLOT(onSaveAsClick()));
    actionSaveAs->setShortcut(QKeySequence::SaveAs);
    auto actionSnapshot = barEditor->addAction(QIcon(":buttons/camera"), tr("Snapshot"), this, SLOT(onSnapshotClick()));
    actionSnapshot->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_H));

#ifndef WITH_TOUCH
    layoutMain->addWidget(barEditor);
#endif

    videoWidget = new QGst::Ui::VideoWidget;
    layoutMain->addWidget(videoWidget);
    videoWidget->setMinimumSize(videoSize);
    videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto barMediaControls = new QToolBar(tr("Media"));
    barMediaControls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    lblStart = new QLabel;
    lblStart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lblStart->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    setLabelTime(0, lblStart);
    barMediaControls->addWidget(lblStart);

    auto actionSetLower = barMediaControls->addAction(QIcon(":buttons/cut_left"), tr("Head"), this, SLOT(onCutClick()));
    actionSetLower->setData(-1);
    actionSetLower->setShortcut(QKeySequence(Qt::AltModifier | Qt::Key_Left));
    actionSeekBack = barMediaControls->addAction(QIcon(":buttons/rewind"), tr("Rewing"), this, SLOT(onSeekClick()));
    actionSeekBack->setData(-frameDuration);
    actionSeekBack->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Left));

    lblCurr = new QLabel;
    lblCurr->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lblCurr->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    setLabelTime(0, lblCurr);
    barMediaControls->addWidget(lblCurr);

    actionPlay = barMediaControls->addAction(QIcon(":buttons/record"), tr("Play"), this, SLOT(onPlayPauseClick()));
    actionPlay->setShortcut(QKeySequence(Qt::Key_Space));
    actionSeekFwd = barMediaControls->addAction(QIcon(":buttons/forward"),  tr("Forward"), this, SLOT(onSeekClick()));
    actionSeekFwd->setData(+frameDuration);
    actionSeekFwd->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Right));
    auto actionSetUpper = barMediaControls->addAction(QIcon(":buttons/cut_right"), tr("Tail"), this, SLOT(onCutClick()));
    actionSetUpper->setData(+1);
    actionSetUpper->setShortcut(QKeySequence(Qt::AltModifier | Qt::Key_Right));
    lblStop = new QLabel;
    lblStop->setAlignment(Qt::AlignLeft | Qt::AlignCenter);
    lblStop->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setLabelTime(0, lblStop);
    barMediaControls->addWidget(lblStop);
    layoutMain->addWidget(barMediaControls, 0, Qt::AlignJustify);

    sliderPos = new QSlider(Qt::Horizontal);
    sliderPos->setTickPosition(QSlider::TicksAbove);
    sliderPos->setTickInterval(SLIDER_SCALE / 100);
    sliderPos->setMaximum(SLIDER_SCALE);
    layoutMain->addWidget(sliderPos);
    connect(sliderPos, SIGNAL(sliderMoved(int)), this, SLOT(setPlayerPosition(int)));

    sliderRange = new QxtSpanSlider(Qt::Horizontal);
    sliderRange->setTickPosition(QSlider::TicksBelow);
    sliderRange->setTickInterval(SLIDER_SCALE / 100);
    sliderRange->setMaximum(SLIDER_SCALE);
    sliderRange->setUpperValue(SLIDER_SCALE);
    connect(sliderRange, SIGNAL(lowerPositionChanged(int)), this, SLOT(setLowerPosition(int)));
    connect(sliderRange, SIGNAL(upperPositionChanged(int)), this, SLOT(setUpperPosition(int)));
    layoutMain->addWidget(sliderRange);

#ifdef WITH_TOUCH
    layoutMain->addWidget(barEditor);
#endif
    setLayout(layoutMain);

    QSettings settings;
    restoreGeometry(settings.value("video-editor-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("video-editor-state").toInt());
    setWindowTitle(tr("Video editor"));

    DamnQtMadeMeDoTheSunsetByHands(barEditor);
    DamnQtMadeMeDoTheSunsetByHands(barMediaControls);

    if (!filePath.isEmpty())
    {
        QTimer::singleShot(0, this, SLOT(loadFile()));
    }

    startTimer(100);
}

VideoEditor::~VideoEditor()
{
    if (pipeline)
    {
        pipeline->setState(QGst::StateNull);
        videoWidget->stopPipelineWatch();
    }
}

void VideoEditor::closeEvent(QCloseEvent *evt)
{
    QSettings settings;
    settings.setValue("video-editor-geometry", saveGeometry());
    settings.setValue("video-editor-state", (int)windowState() & ~Qt::WindowMinimized);
    QWidget::closeEvent(evt);
}

void VideoEditor::timerEvent(QTimerEvent *)
{
    if (duration >0LL && pipeline && !sliderPos->isSliderDown() )
    {
        QGst::PositionQueryPtr queryPos = QGst::PositionQuery::create(QGst::FormatTime);
        if (pipeline->query(queryPos))
        {
            auto pos = queryPos->position();
            if (pos > duration)
            {
                pos = duration;
            }
            //qDebug() << pos << SLIDER_SCALE << duration << pos * SLIDER_SCALE / duration;
            sliderPos->setValue(pos * SLIDER_SCALE / duration);

            setLabelTime(pos, lblCurr);
        }
    }
}

void VideoEditor::loadFile()
{
    loadFile(filePath);
}

void VideoEditor::loadFile(const QString& filePath)
{
    qDebug() << filePath;

    if (!pipeline)
    {
        pipeline = QGst::ElementFactory::make("playbin2").dynamicCast<QGst::Pipeline>();
        if (pipeline)
        {
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
        duration = 0LL;
        pipeline->setProperty("uri", QUrl::fromLocalFile(filePath).toEncoded());
        auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(pipeline.staticCast<QGst::Bin>(), details, qApp->applicationName().append(".video-edit-preview").toUtf8());
        pipeline->setState(QGst::StatePaused);
        setWindowTitle(tr("Video editor - %1").arg(filePath));
        this->filePath = filePath;
#ifdef Q_OS_WIN
        // Replace back slashes with regular ones to make gnlfilesource work.
        //
        this->filePath = this->filePath.replace('\\','/');
#endif
    }
}

void VideoEditor::onBusMessage(const QGst::MessagePtr& message)
{
    qDebug() << message->typeName() << " " << message->source()->property("name").toString();
    switch (message->type())
    {
    case QGst::MessageStateChanged:
        onStateChange(message.staticCast<QGst::StateChangedMessage>());
        break;
    case QGst::MessageAsyncDone:
        {
            if (!duration && pipeline)
            {
                // Here we query the pipeline about the content's duration
                // and we request that the result is returned in time format
                //
                QGst::DurationQueryPtr queryLen = QGst::DurationQuery::create(QGst::FormatTime);

                duration = pipeline->query(queryLen)? queryLen->duration(): 0LL;
                setLabelTime(duration, lblStop);

                QGst::PositionQueryPtr queryPos = QGst::PositionQuery::create(QGst::FormatTime);
                if (pipeline->query(queryPos) && duration > 0LL)
                {
                    auto min = queryPos->position() * SLIDER_SCALE / duration;
                    sliderPos->setMinimum(min);
                    sliderRange->setMinimum(min);
                }
            }
        }
        break;
    case QGst::MessageEos:
        if (pipeline && duration)
        {
            // Rewind to the start and pause the video
            //
            QGst::SeekEventPtr evt = QGst::SeekEvent::create(
                1.0, QGst::FormatTime, QGst::SeekFlagFlush,
                QGst::SeekTypeSet, sliderRange->lowerValue() * duration / SLIDER_SCALE,
                QGst::SeekTypeNone, QGst::ClockTime::None
                );

            pipeline->sendEvent(evt);
            pipeline->setState(QGst::StatePaused);
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
                message->source()->setProperty("enable-last-buffer", true);
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
#ifndef Q_OS_WIN
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
        qDebug() << message->typeName() << " " << message->source()->property("name").toString() << " " << message->oldState() << " => " << message->newState();
        if (message->oldState() == QGst::StateReady && message->newState() == QGst::StatePaused)
        {
            // Time to adjust framerate
            //
            gint numerator = 0, denominator = 0;
            auto sink = videoWidget->videoSink();
            if (sink)
            {
                auto pad = sink->getStaticPad("sink");
                if (pad)
                {
                    auto caps = pad->negotiatedCaps();
                    if (caps)
                    {
                        auto s = caps->internalStructure(0);
                        gst_structure_get_fraction (s.data()->operator const GstStructure *(), "framerate", &numerator, &denominator);
                    }
                }
            }

            if (denominator > 0 && numerator > 0)
            {
                frameDuration = (GST_SECOND * denominator) / numerator + 1;
                //qDebug() << "Framerate " << denominator << "/" << numerator << " duration" << frameDuration;
                actionSeekFwd->setData((int)frameDuration);
                actionSeekBack->setData((int)-frameDuration);
            }
        }

        // Make sure the play/pause button is in consistent state
        //
        actionPlay->setIcon(QIcon(message->newState() == QGst::StatePlaying? ":buttons/pause": ":buttons/record"));
        actionPlay->setText(message->newState() == QGst::StatePlaying? tr("Pause"): tr("Play"));
    }
}

void VideoEditor::setLowerPosition(int position)
{
    setPosition(position, lblStart);
}

void VideoEditor::setUpperPosition(int position)
{
    setPosition(position, lblStop);
}

void VideoEditor::setPlayerPosition(int position)
{
    setPosition(position, lblCurr);
}

void VideoEditor::setPosition(int position, QLabel* lbl)
{
    qint64 seek = 0LL;

    if (pipeline && duration > 0LL)
    {
        seek = duration * position / SLIDER_SCALE;
        QGst::SeekEventPtr evt = QGst::SeekEvent::create(
            1.0, QGst::FormatTime, QGst::SeekFlagFlush,
            QGst::SeekTypeSet, seek,
            QGst::SeekTypeNone, QGst::ClockTime::None
        );

        pipeline->sendEvent(evt);
    }

    setLabelTime(seek, lbl);
}

void VideoEditor::setLabelTime(qint64 time, QLabel* lbl)
{
    auto text = QGst::ClockTime(time).toTime().toString("hh:mm:ss.zzz");
    lbl->setText(text);
}

void VideoEditor::onSaveClick()
{
    QTemporaryFile tmpFile;
    auto confirm = QMessageBox::question(this, tr("Video editor"), tr("Overwrite %1?").arg(filePath), QMessageBox::Yes, QMessageBox::No);
    if (confirm == QMessageBox::Yes && exportVideo(&tmpFile))
    {
        QFile::remove(filePath); // Make Microsoft (R) Windows (TM) happy
        QFile::rename(tmpFile.fileName(), filePath);
        tmpFile.setAutoRemove(false);
    }
}

void VideoEditor::onSaveAsClick()
{
    QWaitCursor wait(this);
    QFileDialog dlg(this);
    QFileInfo   fi(filePath);

    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setFileMode(QFileDialog::AnyFile);
    auto filters = dlg.nameFilters();
    auto suffix = fi.suffix();
    filters.insert(0, QString("*.").append(suffix));
    dlg.setDefaultSuffix(suffix);
    dlg.setNameFilters(filters);
    dlg.setDirectory(fi.absolutePath());
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

bool VideoEditor::exportVideo(QFile* outFile)
{
    QSettings settings;
    auto encoder         = settings.value("video-encoder", DEFAULT_VIDEO_ENCODER).toString();
    auto colorConverter =  settings.value("color-converter", "ffmpegcolorspace").toString();
    auto fixColor        = settings.value(encoder + "-colorspace").toBool()? colorConverter + " ! ": "";
    auto encoderParams   = settings.value(encoder + "-parameters").toString();
    auto muxer           = settings.value("video-muxer", DEFAULT_VIDEO_MUXER).toString();
    auto videoEncBitrate = settings.value("bitrate", DEFAULT_VIDEOBITRATE).toInt();

    if (!outFile->open(QIODevice::WriteOnly))
    {
        // TODO
        return false;
    }
    outFile->close();
    qDebug() << outFile->fileName();
    qint64 start = duration * sliderRange->lowerValue() / SLIDER_SCALE;
    qint64 len   = duration * sliderRange->upperValue() / SLIDER_SCALE - start;

    try
    {
        auto pipeDef = QString("gnlfilesource location=\"%1\" start=%2 duration=%3 media-start=0 media-duration=%3 !"
            " %4 %5 name=videoencoder %6 ! %7 ! filesink location=\"%8\"")
            .arg(filePath).arg(start).arg(len).arg(fixColor, encoder, encoderParams, muxer).arg(outFile->fileName());
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

        auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(pipelineExport.staticCast<QGst::Bin>(), details, qApp->applicationName().append(".video-edit-export").toUtf8());

        auto text = tr("Saving %1").arg(outFile->inherits("QTemporaryFile")? filePath: outFile->fileName());
        VideoEncodingProgressDialog dlgProgress(pipelineExport, duration, this);
        dlgProgress.setLabelText(text);
        dlgProgress.setRange(sliderRange->lowerValue(), sliderRange->upperValue());
        pipelineExport->setState(QGst::StatePlaying);
        auto retCode = dlgProgress.exec();

        pipelineExport->setState(QGst::StateNull);
        pipelineExport->getState(nullptr, nullptr, GST_SECOND * 10); // 10 sec
        return retCode == QDialog::Accepted;
    }
    catch (const QGlib::Error& ex)
    {
        const QString msg = ex.message();
        qCritical() << msg;
        QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
    }

    return false;
}

void VideoEditor::onSeekClick()
{
    QGst::PositionQueryPtr queryPos = QGst::PositionQuery::create(QGst::FormatTime);
    if (pipeline && pipeline->query(queryPos))
    {
        auto newPos = queryPos->position() + static_cast<QAction*>(sender())->data().toInt();

        if (newPos >= 0 && newPos < duration)
        {
            QGst::SeekEventPtr evt = QGst::SeekEvent::create(
                 1.0, QGst::FormatTime, QGst::SeekFlagFlush,
                 QGst::SeekTypeSet, newPos,
                 QGst::SeekTypeCur, frameDuration
             );

            pipeline->sendEvent(evt);
        }
    }
}

void VideoEditor::onPlayPauseClick()
{
    if (pipeline)
    {
        if (pipeline->currentState() == QGst::StatePlaying)
        {
            pipeline->setState(QGst::StatePaused);
        }
        else
        {
            QGst::SeekEventPtr evt = QGst::SeekEvent::create(
                1.0, QGst::FormatTime, QGst::SeekFlagFlush,
                QGst::SeekTypeSet, sliderRange->lowerValue() * duration / SLIDER_SCALE,
                QGst::SeekTypeSet, sliderRange->upperValue() * duration / SLIDER_SCALE + frameDuration
                );
            pipeline->sendEvent(evt);
            pipeline->setState(QGst::StatePlaying);
        }
    }
}

void VideoEditor::onCutClick()
{
    QGst::PositionQueryPtr queryPos = QGst::PositionQuery::create(QGst::FormatTime);
    if (pipeline && pipeline->query(queryPos))
    {
        auto pos = queryPos->position();
        auto value = pos * SLIDER_SCALE / duration;
        auto handle = static_cast<QAction*>(sender())->data().toInt();
        if (handle < 0)
        {
            sliderRange->setLowerValue(value);
            setLabelTime(pos, lblStart);
        }
        else if (handle > 0)
        {
            sliderRange->setUpperValue(value);
            setLabelTime(pos, lblStop);
        }
    }
}

void VideoEditor::onSnapshotClick()
{
    QImage img;

    auto frame = pipeline->property("frame");
    if (frame)
    {
        qDebug() << frame.type();
        auto buffer = frame.get<QGst::BufferPtr>();
        if (buffer)
        {
            auto caps = buffer->caps();
            qDebug() << caps << " size " << buffer->size();

            auto structure = caps->internalStructure(0);
            auto width = structure.data()->value("width").get<int>();
            auto height = structure.data()->value("height").get<int>();

            if (qstrcmp(structure.data()->name().toLatin1(), "video/x-raw-yuv") == 0)
            {
                QGst::Fourcc fourcc = structure->value("format").get<QGst::Fourcc>();
                qDebug() << "fourcc: " << fourcc.value.as_integer;
                if (fourcc.value.as_integer == QGst::Fourcc("I420").value.as_integer)
                {
                    img = QImage(width/2, height/2, QImage::Format_RGB32);

                    auto data = buffer->data();

                    for (int y = 0; y < height; y += 2)
                    {
                        auto yLine = data + y*width;
                        auto uLine = data + width*height + y*width/4;
                        auto vLine = data + width*height*5/4 + y*width/4;

                        for (int x=0; x<width; x+=2)
                        {
                            auto Y = 1.164*(yLine[x]-16);
                            auto U = uLine[x/2]-128;
                            auto V = vLine[x/2]-128;

                            int b = qBound(0, int(Y + 2.018*U), 255);
                            int g = qBound(0, int(Y - 0.813*V - 0.391*U), 255);
                            int r = qBound(0, int(Y + 1.596*V), 255);

                            img.setPixel(x/2,y/2,qRgb(r,g,b));
                        }
                    }
                }
                else
                {
                    qDebug() << "Unsupported yuv pixel format";
                }
            }
            else if (qstrcmp(structure.data()->name().toLatin1(), "video/x-raw-rgb") == 0)
            {
                auto bpp = structure.data()->value("bpp").get<int>();
                qDebug() << "RGB " << bpp;

                auto format = bpp == 32? QImage::Format_RGB32:
                              bpp == 24? QImage::Format_RGB888:
                              QImage::Format_Invalid;

                if (format != QImage::Format_Invalid)
                {
                    img = QImage(buffer->data(), width, height, format);
                }
                else
                {
                    qDebug() << "Unsupported rgb bpp";
                }
            }
        }
    }

    if (img.isNull())
    {
        img = QPixmap::grabWidget(videoWidget).toImage();
    }

    if (!img.isNull())
    {
        QWaitCursor wait(this);
        QFileDialog dlg(this);
        dlg.setAcceptMode(QFileDialog::AcceptSave);
        dlg.setFileMode(QFileDialog::AnyFile);
        auto filters = dlg.nameFilters();
        filters.insert(0, "*.jpg");
        dlg.setDefaultSuffix("jpg");
        dlg.setNameFilters(filters);
        dlg.setDirectory(QFileInfo(filePath).absolutePath());
        if (dlg.exec())
        {
            img.save(dlg.selectedFiles().first(), "JPG");
        }
    }
}
