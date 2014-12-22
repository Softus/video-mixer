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

#include "pipeline.h"
#include "defaults.h"
#include "typedetect.h"
#include "settings/videosources.h"

#include <QApplication>

#include <QGlib/Connect>

#include <QGst/Bus>
#include <QGst/ElementFactory>
#include <QGst/Parse>

#include <gst/gst.h>
#if GST_CHECK_VERSION(1,0,0)
#include <QGst/VideoOverlay>
#endif

// From Gstreamer SDK
//
#include <gst/gstdebugutils.h>
#include <gst/interfaces/tuner.h>

Pipeline::Pipeline(int index, QObject *parent) :
    QObject(parent),
    index(index),
    motionDetected(false),
    motionStart(false),
    motionStop(false),
    recording(false)
{
    displayWidget = new VideoWidget();
    displayWidget->setProperty("index", index);
}

Pipeline::~Pipeline()
{
    if (pipeline)
    {
        releasePipeline();
    }

    delete displayWidget;
}

void Pipeline::releasePipeline()
{
    pipeline->setState(QGst::StateNull);
    pipeline->getState(nullptr, nullptr, 10000000000L); // 10 sec
    motionDetected = false;

    displaySink.clear();
    imageValve.clear();
    imageSink.clear();
    videoEncoder.clear();
    displayOverlay.clear();
    displayWidget->stopPipelineWatch();
    pipeline.clear();
}

/*

The pipeline for software encoder:

                 [video src #]
                      |
                      V
          [video decoder, for DV/JPEG #]
                      |
                      V
                [deinterlace #]
                      |
                      V
         +----[main splitter]------+
         |            |            |
  [image valve]   [detector]  [video rate]
         |            |            |
         V            V            V
 [image encoder]  [display]   [video valve]
         |                         |
         V                         V
  [image writer]             [video encoder]
                                   |
                                   V
                       +----[video splitter]----+---------------+
                       |           |            |               |
                       V           V            V               V
            [movie writer]   [clip valve]  [rtp sender #] [http sender #]
                                   |
                                   V
                            [clip writer]


Sample:
    v4l2src [! dvdemux ! ffdec_dvvideo | ! jpegdec] [! colorspace] [! deinterlace] ! tee name=splitter
        splitter. ! autovideosink name=displaysink async=0
        splitter. ! valve name=encvalve drop=1 ! queue max-size-bytes=0 ! videorate max-rate=30/1 ! x264enc name=videoencoder ! tee name=videosplitter
                videosplitter. ! identity  name=videoinspect drop-probability=1.0 ! queue ! valve name=videovalve drop=1 ! [mpegpsmux name=videomux ! filesink name=videosink]
                videosplitter. ! queue ! rtph264pay ! udpsink name=rtpsink clients=127.0.0.1:5000 sync=0
                videosplitter. ! identity  name=clipinspect drop-probability=1.0 ! queue ! valve name=clipvalve ! [ mpegpsmux name=clipmux ! filesink name=clipsink]
        splitter. ! identity name=imagevalve drop-probability=1.0 ! jpegenc ! multifilesink name=imagesink post-messages=1 async=0 sync=0 location=/video/image

The pipeline for hardware encoder:

                [video src #]
                     |
                     V
         +----[video splitter #]----+----------+------------+
         |           |              |          |            |
         V           V              V          V            V
[movie writer] [clip valve] [rtp sender #] [decoder #] [http sender #]
                     |                        |
                     V                        V
               [clip writer]             [splitter]-------+
                                              |           |
                                              V           V
                                       [image valve]  [detector]
                                              |           |
                                              V           V
                                      [image encoder] [display]
                                              |
                                              V
                                      [image writer]
*/

QString appendVideo(QString& pipe, const QSettings& settings, bool enableVideoLog)
{
/*
                       +----[video splitter]----+-------------+
                       |           |            |             |
                       V           V            V             V
            [movie writer]   [clip valve]  [rtp sender]  [http sender]
                                   |
                                   V
                            [clip writer]

*/
    auto rtpPayDef       = settings.value("rtp-payloader",  DEFAULT_RTP_PAYLOADER).toString();
    auto rtpPayParams    = settings.value(rtpPayDef + "-parameters").toString();
    auto rtpSinkDef      = settings.value("rtp-sink",       DEFAULT_RTP_SINK).toString();
    auto rtpSinkParams   = settings.value(rtpSinkDef + "-parameters").toString();

    auto rtpClients      = settings.value("rtp-clients").toString();
    auto enableRtp       = !rtpSinkDef.isEmpty() && !rtpClients.isEmpty()
                            && settings.value("enable-rtp").toBool();
    auto httpSinkDef     = settings.value("http-sink",      DEFAULT_HTTP_SINK).toString();
    auto enableHttp      = !httpSinkDef.isEmpty() && settings.value("enable-http").toBool();
    auto httpPushUrl     = settings.value("http-push-url").toString();
    auto httpSinkParams  = settings.value(httpSinkDef + "-parameters").toString();

    pipe.append(" ! tee name=videosplitter");
    if (enableRtp || enableHttp || enableVideoLog)
    {
        if (enableVideoLog)
        {
            pipe.append("\nvideosplitter. ! identity name=videoinspect drop-probability=1.0 ! queue ! valve name=videovalve ");
        }

        if (enableRtp)
        {
            pipe.append("\nvideosplitter. ! queue ! ");

            // MPEG2TS is a payloader for container, so add the required muxer
            //
            if (rtpPayDef == "rtpmp2tpay")
            {
                pipe.append("mpegtsmux name=rtpmux ! ");
            }

            pipe.append(rtpPayDef).append(" ").append(rtpPayParams)
                .append(" ! ").append(rtpSinkDef).append(" clients=\"").append(rtpClients)
                .append("\" sync=0 async=0 name=rtpsink ")
                .append(rtpSinkParams);
        }

        if (enableHttp && !httpPushUrl.isEmpty())
        {
            pipe.append("\nvideosplitter. ! queue ! mpegtsmux name=httpmux ! ").append(httpSinkDef)
                .append(" async=0 name=httpsink location=\"").append(httpPushUrl).append("\" ").append(httpSinkParams);
        }
    }

    return pipe.append("\nvideosplitter. ! identity name=clipinspect drop-probability=1.0 ! queue ! valve name=clipvalve drop=1");
}

QString buildDetectMotion(const QSettings& settings)
{
    QString pipe;
    auto detectMotion = settings.value("detect-motion", DEFAULT_MOTION_DETECTION).toBool();

    if (detectMotion)
    {
        auto motionDebug       = settings.value("motion-debug", false).toString();
        auto motionSensitivity = settings.value("motion-sensitivity", DEFAULT_MOTION_SENSITIVITY).toString();
        auto motionThreshold   = settings.value("motion-threshold",   DEFAULT_MOTION_THRESHOLD).toString();
        auto motionMinFrames   = settings.value("motion-min-frames",  DEFAULT_MOTION_MIN_FRAMES).toString();
        auto motionGap         = settings.value("motion-gap",         DEFAULT_MOTION_GAP).toString();

        pipe.append(" ! motioncells name=motion-detector display=").append(motionDebug)
            .append(" sensitivity=").append(motionSensitivity)
            .append(" threshold=").append(motionThreshold)
            .append(" minimummotionframes=").append(motionMinFrames)
            .append(" gap=").append(motionGap);
    }
    return pipe;
}

QString Pipeline::buildPipeline(const QSettings &settings, const QString &outputPathDef,
                                bool enableVideoLog, const QString &detectMotion)
{
    // v4l2src device=/dev/video1 name=(channel) ! video/x-raw-yuv,format=YUY2,width=720,height=576 ! colorspace
    // dv1394src guid="9025895599807395" ! video/x-dv,format=PAL ! dvdemux ! dvdec ! colorspace
    //
    QString pipe;

    auto deviceDef      = settings.value("device").toString();
    auto deviceType     = settings.value("device-type", PLATFORM_SPECIFIC_SOURCE).toString();
    auto inputChannel   = settings.value("video-channel").toString();
    auto formatDef      = settings.value("format").toString();
    auto sizeDef        = settings.value("size").toSize();
    auto srcDeinterlace = settings.value("video-deinterlace").toBool();
    auto srcParams      = settings.value("src-parameters").toString();
    alias               = settings.value("alias").toString();
    modality            = settings.value("modality").toString();

    if (alias.isEmpty())
    {
        alias = QString("src%1").arg(index);
    }

    auto colorConverter = QString(" ! ").append(settings.value("color-converter", "ffmpegcolorspace").toString());
    auto videoCodec     = settings.value("video-encoder",  DEFAULT_VIDEO_ENCODER).toString();
    auto bitrate        = settings.value("bitrate").toString();

    pipe.append(deviceType);

    if (deviceType == "dv1394src")
    {
        // Special handling of dv video sources
        //
        if (inputChannel.toInt() > 0)
        {
            pipe.append(" channel=").append(inputChannel).append("");
        }
        if (!deviceDef.isEmpty())
        {
            pipe.append(" guid=\"").append(deviceDef).append("\"");
        }
    }
    else if (deviceType == "videotestsrc")
    {
        // Special handling of test video sources
        //
        if (inputChannel.toInt() > 0)
        {
            pipe.append(" pattern=").append(inputChannel).append("");
        }
    }
    else
    {
        if (!inputChannel.isEmpty())
        {
            // Hack: since channel name can't be set with attributes,
            // we set element name instead. The one reason is to
            // make the pipeline text do not match with older one.
            //
            pipe.append(" name=\"").append(inputChannel).append("\"");
        }
        if (!deviceDef.isEmpty())
        {
            pipe.append(" " PLATFORM_SPECIFIC_PROPERTY "=\"").append(deviceDef).append("\"");
        }
    }

    if (!srcParams.isEmpty())
    {
        pipe.append(' ').append(srcParams);
    }

    if (!formatDef.isEmpty())
    {
        pipe.append(" ! ").append(formatDef);
        if (!sizeDef.isEmpty())
        {
            pipe = pipe.append(",width=%1,height=%2").arg(sizeDef.width()).arg(sizeDef.height());
        }
    }

    if (videoCodec.isEmpty())
    {
        appendVideo(pipe, settings, enableVideoLog);
        pipe.append("\nvideosplitter.");
    }

    auto formatType = formatDef.split(',').first();
    if (formatType == "image/jpeg")
    {
        pipe.append(" ! jpegdec");
    }
    else if (deviceType == "dv1394src" || formatType == "video/x-dv")
    {
        // Add dv demuxer & decoder for DV sources
        //
        pipe.append(" ! dvdemux ! dvdec");
    }

    pipe.append(colorConverter).append(srcDeinterlace? " ! deinterlace mode=1 method=4": "");

    // v4l2src ... ! tee name=splitter [! colorspace ! motioncells] ! colorspace ! autovideosink");
    //
    auto displaySinkDef  = settings.value("display-sink", DEFAULT_DISPLAY_SINK).toString();
    auto displayParams   = settings.value(displaySinkDef + "-parameters").toString();

    pipe.append(" ! tee name=splitter");
    if (!displaySinkDef.isEmpty())
    {
        pipe.append("\nsplitter.").append(colorConverter).append(detectMotion)
            .append(" ! textoverlay name=displayoverlay color=-1 halignment=right valignment=top text=* xpad=8 ypad=0 font-desc=16")
            .append(colorConverter)
            .append(" ! " ).append(displaySinkDef).append(" name=displaysink async=0 ").append(displayParams);
    }

    // ... splitter. ! identity name=imagevalve ! jpegenc ! multifilesink splitter.
    //
    auto imageEncoderDef = settings.value("image-encoder", DEFAULT_IMAGE_ENCODER).toString();
    auto imageEncoderFixColor = settings.value(imageEncoderDef + "-colorspace", false).toBool();
    auto imageEncoderParams = settings.value(imageEncoderDef + "-parameters").toString();
    auto imageSinkDef       = settings.value("image-sink", DEFAULT_IMAGE_SINK).toString();
    if (!imageSinkDef.isEmpty())
    {
        pipe.append("\nsplitter. ! identity name=imagevalve drop-probability=1.0")
            .append(imageEncoderFixColor? colorConverter: "")
            .append(" ! ").append(imageEncoderDef).append(" ").append(imageEncoderParams)
            .append(" ! ").append(imageSinkDef).append(" name=imagesink post-messages=1 async=0 sync=0 location=\"")
            .append(outputPathDef).append("/image\"");
    }

    if (!videoCodec.isEmpty())
    {
        // ... splitter. ! videorate ! valve name=encvalve ! colorspace ! x264enc name="4000"
        //           ! tee name=videosplitter
        //                videosplitter. ! queue ! mpegpsmux ! filesink
        //                videosplitter. ! queue ! rtph264pay ! udpsink
        //                videosplitter. ! identity name=clipinspect ! queue ! mpegpsmux ! filesink
        //
        auto videoMaxRate       = settings.value("limit-video-fps", DEFAULT_LIMIT_VIDEO_FPS).toBool()?
                                  settings.value("video-max-fps",  DEFAULT_VIDEO_MAX_FPS).toInt(): 0;
        auto videoFixColor      = settings.value(videoCodec + "-colorspace").toBool();
        auto videoEncoderParams = settings.value(videoCodec + "-parameters").toString();
        auto noIdleStream       = settings.value("no-idle-stream").toBool();

        pipe.append("\nsplitter.");
        if (videoMaxRate > 0)
        {
            pipe.append(" ! videorate skip-to-first=1 max-rate=").append(QString::number(videoMaxRate));
        }

        if (noIdleStream)
        {
            pipe.append(" ! valve name=encvalve drop=1");
        }

        // Store encoder bitrate as the queue name to detect pipeline changes.
        // The real bitrate will be set later, after we figure out is it bits or kbits
        //
        pipe.append(" ! queue name=").append(bitrate).append(" max-size-bytes=0")
            .append(videoFixColor? colorConverter: "")
            .append(" ! ").append(videoCodec).append(" name=videoencoder ").append(videoEncoderParams);

        appendVideo(pipe, settings, enableVideoLog);
    }

    return pipe;
}

bool Pipeline::updatePipeline()
{
    QSettings settings;
    auto outputPathDef  = settings.value("storage/output-path",    DEFAULT_OUTPUT_PATH).toString();

    settings.beginGroup("gst");
    auto enableVideoLog = settings.value("enable-video").toBool();
    auto detectMotion = enableVideoLog? buildDetectMotion(settings): QString();

    if (index >= 0)
    {
        settings.beginReadArray("src");
        settings.setArrayIndex(index);
    }

    auto newPipelineDef = buildPipeline(settings, outputPathDef, enableVideoLog, detectMotion);
    if (newPipelineDef == pipelineDef)
    {
        return false;
    }

    qDebug() << "The pipeline has been changed, restarting";
    if (pipeline)
    {
        releasePipeline();
    }

    qCritical() << newPipelineDef;
    try
    {
        pipeline = QGst::Parse::launch(newPipelineDef).dynamicCast<QGst::Pipeline>();
    }
    catch (const QGlib::Error& ex)
    {
        errorGlib(pipeline, ex);
        return false;
    }

    pipelineDef = newPipelineDef;

    // Connect to the underlying hardware
    //
    pipeline->setState(QGst::StateReady);

    auto videoInputChannel = settings.value("video-channel").toString();
    if (!videoInputChannel.isEmpty())
    {
        auto tuner = GST_TUNER(gst_bin_get_by_interface(pipeline.staticCast<QGst::Bin>(), GST_TYPE_TUNER));
        if (tuner)
        {
            auto walk = (GList *)gst_tuner_list_channels(tuner);
            while (walk)
            {
                auto ch = GST_TUNER_CHANNEL(walk->data);
                if (0 == videoInputChannel.compare(ch->label))
                {
                    gst_tuner_set_channel(tuner, ch);
                    break;
                }
                walk = g_list_next (walk);
            }

            g_object_unref(tuner);
        }
    }

    if (index >= 0)
    {
        settings.endArray();
    }

    pipeline->bus()->addSignalWatch();
    displayWidget->watchPipeline(pipeline);
    QGlib::connect(pipeline->bus(), "message", this, &Pipeline::onBusMessage);

    displaySink    = pipeline->getElementByName("displaysink");
    displayOverlay = pipeline->getElementByName("displayoverlay");
    videoEncoder   = pipeline->getElementByName("videoencoder");
    imageValve     = pipeline->getElementByName("imagevalve");
    imageSink      = pipeline->getElementByName("imagesink");

    if (!displaySink)
    {
        qCritical() << "Element displaysink not found";
    }

    if (!imageSink)
    {
        qCritical() << "Element imagesink not found";
    }

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

        videoEncoder->setProperty("bitrate", videoEncBitrate);
        qDebug() << "video bitrate" << videoEncoder->property("bitrate").toInt();
    }

    imageValve && QGlib::connect(imageValve, "handoff", this, &Pipeline::onImageReady);

    auto clipInspect = pipeline->getElementByName("clipinspect");
    clipInspect && QGlib::connect(clipInspect, "handoff", this, &Pipeline::onClipFrame);

    auto videoInspect = pipeline->getElementByName("videoinspect");
    videoInspect && QGlib::connect(videoInspect, "handoff", this, &Pipeline::onVideoFrame);

    auto flags = GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES;
    auto details = GstDebugGraphDetails(flags);
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(pipeline.staticCast<QGst::Bin>(), details, qApp->applicationName().toUtf8());

    motionStart  = !detectMotion.isEmpty() && settings.value("motion-start", DEFAULT_MOTION_START).toBool();
    motionStop   = !detectMotion.isEmpty() && settings.value("motion-stop", DEFAULT_MOTION_STOP).toBool();

    // Start the pipeline
    pipeline->setState(QGst::StatePlaying);

    settings.endGroup();
    return true;
}

void Pipeline::errorGlib(const QGlib::ObjectPtr& obj, const QGlib::Error& ex)
{
    const QString msg = obj?
        QString().append(obj->property("name").toString()).append(" ").append(ex.message()):
        ex.message();
    qCritical() << msg;
    pipelineError(msg);
}

void Pipeline::setElementProperty(const char* elmName, const char* prop, const QGlib::Value& value, QGst::State minimumState)
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

void Pipeline::setElementProperty(QGst::ElementPtr& elm, const char* prop, const QGlib::Value& value, QGst::State minimumState)
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
            //qDebug() << elm->name() << prop << value.toString();
            elm->setProperty(prop, value);
        }
        elm->setState(currentState);
        elm->getState(nullptr, nullptr, 1000000000L);
    }
}

static QString getExt(QString str)
{
    if (str.startsWith("ffmux_"))
    {
        str = str.mid(6);
    }
    return QString(".").append(str.remove('e').left(3));
}

QString Pipeline::appendVideoTail(const QDir& dir, const QString& prefix, QString clipFileName, bool split)
{
    QSettings settings;
    settings.beginGroup("gst");
    if (index >= 0)
    {
        settings.beginReadArray("src");
        settings.setArrayIndex(index);
    }

    auto muxDef  = settings.value("video-muxer",    DEFAULT_VIDEO_MUXER).toString();

    if (index >= 0)
    {
        settings.endArray();
    }

    auto maxSize = split? settings.value("video-max-file-size", DEFAULT_VIDEO_MAX_FILE_SIZE).toLongLong() * 1024 * 1024: 0;
    settings.endGroup();

    QString videoExt;
    split = maxSize > 0;

    QGst::ElementPtr mux;
    auto valve   = pipeline->getElementByName((prefix + "valve").toUtf8());
    if (!valve)
    {
        qDebug() << "Required element '" << prefix + "valve'" << " is missing";
        return nullptr;
    }

    if (!muxDef.isEmpty())
    {
        mux = QGst::ElementFactory::make(muxDef, (prefix + "mux").toUtf8());
        if (!mux)
        {
            qDebug() << "Failed to create element '" << prefix + "mux'" << " (" << muxDef << ")";
            return nullptr;
        }
        pipeline->add(mux);
    }

    QGst::ElementPtr sink;
    if (!split)
    {
        sink = QGst::ElementFactory::make("filesink", (prefix + "sink").toUtf8());
    }
    else
    {
        sink = QGst::ElementFactory::make("multifilesink", (prefix + "sink").toUtf8());
        if (sink && sink->findProperty("max-file-size"))
        {
            sink->setProperty("post-messages", true);
        }
        else
        {
            split = false;
            qDebug() << "multiflesink does not support 'max-file-size' property, replaced with filesink";
            sink = QGst::ElementFactory::make("filesink", (prefix + "sink").toUtf8());
        }
    }

    if (!sink)
    {
        qDebug() << "Failed to create filesink element";
        return nullptr;
    }

    pipeline->add(sink);

    if (!mux)
    {
        if (!valve->link(sink))
        {
            qDebug() << "Failed to link elements altogether";
            return nullptr;
        }
        videoExt = ".mpg";
    }
    else
    {
        if (!QGst::Element::linkMany(valve, mux, sink))
        {
            qDebug() << "Failed to link elements altogether";
            return nullptr;
        }
        videoExt = getExt(muxDef);
    }

    // Manually increment video/clip file name
    //
    clipFileName.replace("%src%", alias).append(split? "%02d": "").append(videoExt);
    auto absPath = dir.absoluteFilePath(clipFileName);
    sink->setProperty("location", absPath);
    if (split)
    {
        sink->setProperty("next-file", 4);
        sink->setProperty("max-file-size", maxSize);
    }
    mux && mux->setState(QGst::StatePaused);
    sink->setState(QGst::StatePaused);
    valve->setProperty("drop", true);

    // Replace '%02d' with '00' to get the real clip name
    //
    auto realFileName = split? absPath.replace("%02d","00"): absPath;
    if (!modality.isEmpty())
    {
        SetFileExtAttribute(realFileName, "modality", modality);
    }
    return realFileName;
}

void Pipeline::removeVideoTail(const QString& prefix)
{
    if (!pipeline)
    {
        return;
    }

    auto inspect = pipeline->getElementByName((prefix + "inspect").toUtf8());
    auto valve   = pipeline->getElementByName((prefix + "valve").toUtf8());
    auto mux     = pipeline->getElementByName((prefix + "mux").toUtf8());
    auto sink    = pipeline->getElementByName((prefix + "sink").toUtf8());

    if (!sink)
    {
        return;
    }

    inspect->setProperty("drop-probability", 1.0);
    valve->setProperty("drop", true);

    sink->setState(QGst::StateNull);
    sink->getState(nullptr, nullptr, 1000000000L);
    if (mux)
    {
        mux->setState(QGst::StateNull);
        mux->getState(nullptr, nullptr, 1000000000L);
        QGst::Element::unlinkMany(valve, mux, sink);
        pipeline->remove(mux);
    }
    else
    {
        valve->unlink(sink);
    }
    pipeline->remove(sink);

    if (!modality.isEmpty())
    {
        auto realFileName = sink->property("location").toString().replace("%02d","00");
        SetFileExtAttribute(realFileName, "modality", modality);
    }
}

void Pipeline::updateOverlayText(int countdown)
{
    if (!displayOverlay)
        return;

    QString text;
    if (recording)
    {
        if (countdown > 0)
            text.setNum(countdown);
        else
            text.append('*');
    }

    auto videovalve = pipeline->getElementByName("videovalve");
    auto videosink  = pipeline->getElementByName("videosink");
    if (videosink && videovalve && !videovalve->property("drop").toBool())
    {
        text.append(" log");
    }

    text.append(' ').append(alias);

    displayOverlay->setProperty("color", 0xFFFF0000);
    displayOverlay->setProperty("outline-color", 0xFFFF0000);
    displayOverlay->setProperty("text", text);
}

void Pipeline::enableEncoder(bool enable)
{
    setElementProperty("encvalve", "drop", !enable);
}

void Pipeline::enableClip(bool enable)
{
    setElementProperty("clipinspect", "drop-probability", enable? 0.0: 1.0);
}

void Pipeline::enableVideo(bool enable)
{
    setElementProperty("videoinspect", "drop-probability",
        enable && (!motionStart || motionDetected)? 0.0: 1.0);
}

void Pipeline::setImageLocation(QString filename)
{
    if (!imageSink)
    {
        qDebug() << "Failed to set image location: no image sink";
        return;
    }

    // Pay attention to %src% macro, which can not be evaluated till now.
    //
    filename.replace("%src%", alias);

    // Do not bother the pipeline, if the location is the same.
    //
    auto location = imageSink->property("location").toString();
    if (location != filename)
    {
        setElementProperty(imageSink, "location", filename, QGst::StateReady);
    }
}

void Pipeline::onBusMessage(const QGst::MessagePtr& msg)
{
    //qDebug() << msg->typeName() << " " << msg->source()->property("name").toString();

    switch (msg->type())
    {
    case QGst::MessageStateChanged:
        // The display area of the main window is filled with some garbage.
        // We need to redraw the contents.
        //
        if (msg->source() == pipeline)
        {
            foreach (auto w, qApp->topLevelWidgets())
            {
                w->update();
            }
        }
        break;
    case QGst::MessageElement:
        onElementMessage(msg.staticCast<QGst::ElementMessage>());
        break;
    case QGst::MessageError:
        errorGlib(msg->source(), msg.staticCast<QGst::ErrorMessage>()->error());
        break;
#ifdef QT_DEBUG
    case QGst::MessageInfo:
        qDebug() << msg->source()->property("name").toString() << " " << msg.staticCast<QGst::InfoMessage>()->error();
        break;
    case QGst::MessageWarning:
        qDebug() << msg->source()->property("name").toString() << " " << msg.staticCast<QGst::WarningMessage>()->error();
        break;
    case QGst::MessageEos:
    case QGst::MessageNewClock:
    case QGst::MessageStreamStatus:
    case QGst::MessageQos:
    case QGst::MessageAsyncDone:
        break;
    default:
        qDebug() << msg->type();
        break;
#else
    default: // Make the compiler happy
        break;
#endif
    }
}

void Pipeline::onElementMessage(const QGst::ElementMessagePtr& msg)
{
    const QGst::StructurePtr s = msg->internalStructure();
    if (!s)
    {
        qDebug() << "Got empty QGst::MessageElement";
        return;
    }

    if (s->name() == "GstMultiFileSink")
    {
        QString filename = s->value("filename").toString();
        if (msg->source() == imageSink)
        {
            QString tooltip = filename;
            QPixmap pm;

            auto lastBuffer = msg->source()->property("last-buffer").get<QGst::BufferPtr>();
            bool ok = lastBuffer && pm.loadFromData(lastBuffer->data(), lastBuffer->size());

            // If we can not load from the buffer, try to load from the file
            //
            if (!ok && !pm.load(filename))
            {
                tooltip = tr("Failed to load image %1").arg(filename);
                pm.load(":/buttons/stop");
            }

            imageSaved(filename, tooltip, pm);
        }

        if (!modality.isEmpty())
        {
            SetFileExtAttribute(filename, "modality", modality);
        }
        return;
    }

#if GST_CHECK_VERSION(1,0,0)
    if (QGst::VideoOverlay::isPrepareWindowHandleMessage(msg))
#else
    if (s->name() == "prepare-xwindow-id" || s->name() == "prepare-window-handle")
#endif
    {
        // At this time the video output finally has a sink, so set it up now
        //
        msg->source()->setProperty("force-aspect-ratio", true);
        displayWidget->update();
        return;
    }

    if (s->name() == "motion")
    {
        if (motionStart && s->hasField("motion_begin"))
        {
            motionDetected = true;
            if (pipeline->getElementByName("videosink"))
            {
                setElementProperty("videoinspect", "drop-probability", 0.0);
            }
        }
        else if (motionStop && s->hasField("motion_finished"))
        {
            motionDetected = false;
            if (pipeline->getElementByName("videosink"))
            {
                setElementProperty("videoinspect", "drop-probability", 1.0);
                setElementProperty("videovalve", "drop", true);
            }
        }

        motion(motionDetected);
        return;
    }

    qDebug() << "Got unknown message " << s->toString() << " from " << msg->source()->property("name").toString();
}

void Pipeline::onImageReady(const QGst::BufferPtr& buf)
{
    qDebug() << "imageValve handoff " << buf->size() << " " << buf->timeStamp() << " " << buf->flags();
    imageValve->setProperty("drop-probability", 1.0);
    imageReady();
}

void Pipeline::onClipFrame(const QGst::BufferPtr& buf)
{
    if (0 != (buf->flags() & GST_BUFFER_FLAG_DELTA_UNIT))
    {
        return;
    }

    // Once we got an I-Frame, open second valve
    //
    setElementProperty("clipvalve", "drop", false);
    clipFrameReady();
}

void Pipeline::onVideoFrame(const QGst::BufferPtr& buf)
{
    if (0 != (buf->flags() & GST_BUFFER_FLAG_DELTA_UNIT))
    {
        return;
    }

    // Once we got an I-Frame, open second valve
    //
    setElementProperty("videovalve", "drop", false);
    videoFrameReady();
}
