#include "mixer.h"
#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QTimer>

#include <QGlib/Connect>
#include <QGlib/Error>
#include <QGst/Bus>
#include <QGst/Parse>

#include <gst/gst.h>

#define DEFAULT_WIDTH         160
#define DEFAULT_HEIGHT        120
#define DEFAULT_RESTART_DELAY 1000
#define DEFAULT_PADDING       8
#define DEFAULT_MARGINS       QRect(32, 32, 32, 32)
#define DEFAULT_MESSAGE       "No active sources"

#define DEFAULT_SOURCE  "souphttpsrc timeout=1"
#define DEFAULT_SINK    "souphttpclientsink"

#if GST_CHECK_VERSION(1,0,0)
#define VIDEO_XRAW      "video/x-raw"
#define FOURCC_I420     "(string)I420"
#define VIDEOCONVERTER  "videoconvert"
#define DEFAULT_DECODER "tsdemux ! avdec_mpeg2video"
#define DEFAULT_ENCODER "avenc_mpeg2video bitrate=1000000 ! mpegtsmux"
#else
#define VIDEO_XRAW      "video/x-raw-yuv"
#define FOURCC_I420     "(fourcc)I420"
#define VIDEOCONVERTER  "ffmpegcolorspace"
#define DEFAULT_DECODER "mpegtsdemux ! ffdec_mpeg2video"
#define DEFAULT_ENCODER "ffenc_mpeg2video bitrate=1000000 ! mpegtsmux"
#endif

Mixer::Mixer(const QString& group, QObject *parent) :
    QObject(parent), updateTimerId(0)
{
    QSettings settings;

    width     = settings.value("width",   DEFAULT_WIDTH).toInt();
    height    = settings.value("height",  DEFAULT_HEIGHT).toInt();
    delay     = settings.value("delay",   DEFAULT_RESTART_DELAY).toInt();
    padding   = settings.value("padding", DEFAULT_PADDING).toInt();
    margins   = settings.value("margins", DEFAULT_MARGINS).toRect();
    decoder   = settings.value("decoder", DEFAULT_DECODER).toString();
    encoder   = settings.value("encoder", DEFAULT_ENCODER).toString();
    message   = settings.value("message", DEFAULT_MESSAGE).toString();
    source    = settings.value("source",  DEFAULT_SOURCE).toString();
    sink      = settings.value("sink",    DEFAULT_SINK).toString();
    zOrderFix = settings.value("zorderfix", false).toBool();

    auto size = settings.beginReadArray(group);
    dstUri    = settings.value("dst").toString();
    width     = settings.value("width",   width).toInt();
    height    = settings.value("height",  height).toInt();
    delay     = settings.value("delay",   delay).toInt();
    padding   = settings.value("padding", padding).toInt();
    margins   = settings.value("margins", margins).toRect();
    decoder   = settings.value("decoder", decoder).toString();
    encoder   = settings.value("encoder", encoder).toString();
    message   = settings.value("message", message).toString();
    source    = settings.value("source",  source).toString();
    sink      = settings.value("sink",    sink).toString();
    zOrderFix = settings.value("zorderfix", zOrderFix).toBool();

    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        srcMap.insert(settings.value("src").toString(),
            QPair<QString, bool>(settings.value("name").toString(), settings.value("enabled").toBool()));
    }
    settings.endArray();

    // This magic required to start/stop timers from the worker threads on Microsoft (R) Windows (TM)
    //
    connect(this, SIGNAL(restart(int)), this, SLOT(onRestart(int)), Qt::QueuedConnection);

    buildPipeline();
}

Mixer::~Mixer()
{
    releasePipeline();
}

void Mixer::releasePipeline()
{
    if (pl)
    {
        pl->setState(QGst::StateNull);
        if (QGst::StateChangeSuccess != pl->getState(nullptr, nullptr, 5000000000L))
        {
            qDebug() << "Failed to stop the pipeline";
        }

        QGst::BusPtr bus = pl->bus();
        QGlib::disconnect(bus, "message", this, &Mixer::onBusMessage);
        bus->removeSignalWatch();
        bus->disableSyncMessageEmission();

        bus.clear();
        pl.clear();
    }
}

QString Mixer::buildBackground(bool inactive, int rowSize)
{
    QString pipelineDef;

    pipelineDef
        .append("videotestsrc pattern=").append(inactive? "18":"2")
        .append(" is-live=1 do-timestamp=1 ! " VIDEO_XRAW ",format=" FOURCC_I420)
        .append(",width=") .append(QString::number(margins.left() + margins.width()  + width  * rowSize + padding * (rowSize - 1)))
        .append(",height=").append(QString::number(margins.top()  + margins.height() + height * rowSize + padding * (rowSize - 1)))
        ;

    // Add label if all sources are gone
    //
    if (inactive)
    {
        pipelineDef.append(" ! textoverlay halignment=center font-desc=24 text=\"").append(message).append('"');
        restart(60 * 1000); // Restart every 1 min
    }

    pipelineDef.append(" ! videobox border-alpha=0 ! mix.\n");

    return pipelineDef;
}

void Mixer::buildPipeline()
{
    int rowSize = rint(ceil(sqrt(srcMap.size())));
    int top = (srcMap.size() + rowSize - 1) / rowSize - 1, left = (srcMap.size() - 1) % rowSize;
    int streamNo = 0, inactiveStreams = 0;
    QString pipelineDef;

    releasePipeline();

    // Count the number of inactive streams
    //
    for (auto src = srcMap.begin(); src != srcMap.end(); ++src)
    {
        if (!src.value().second)
        {
            inactiveStreams++;
        }
    }

    // Append output
    //
    pipelineDef
        .append("videomixer name=mix ! " VIDEOCONVERTER)
        .append(" ! ").append(encoder)
        .append(" ! queue ! ").append(sink).append(" sync=0 location=").append(dstUri).append("\n");

    if (zOrderFix)
    {
        pipelineDef.append(buildBackground(inactiveStreams == srcMap.size(), rowSize));
    }

    // For each input...
    //
    for (auto src = srcMap.begin(); src != srcMap.end(); ++src)
    {
        // ...if it is still alive...
        //
        if (src.value().second)
        {
            // ...append http source with demuxer and decoder.
            //
            auto text = src.value().first;
            pipelineDef
                .append(source).append(" do-timestamp=1 location=").append(src.key())
                .append(" ! queue ! ").append(decoder).append(" ! videoscale ! " VIDEO_XRAW ",width=")
                    .append(QString::number(width)).append(",height=").append(QString::number(height))
                    .append(" ! " VIDEOCONVERTER " ! textoverlay valignment=bottom halignment=right xpad=2 ypad=2 font-desc=24 text=")
                    .append(text).append(" ! videobox")
                    .append(" left=-").append(QString::number(margins.left() + (padding + width)  * left))
                    .append(" top=-") .append(QString::number(margins.top()  + (padding + height) * top))
                    .append(" ! mix.\n");
        }
        else
        {
            // ...append http source only. And wait for the video stream.
            //
            pipelineDef
                .append(source).append(" location=").append(src.key())
                .append(" ! fakesink name=s").append(QString::number(streamNo++))
                .append(" sync=0 async=0 signal-handoffs=true\n");
        }

        if (--left < 0)
        {
            left = rowSize - 1;
            --top;
        }
    }

    Q_ASSERT(streamNo == inactiveStreams);

    if (!zOrderFix)
    {
        pipelineDef.append(buildBackground(inactiveStreams == srcMap.size(), rowSize));
    }

    qDebug() << pipelineDef;
    try
    {
        pl = QGst::Parse::launch(pipelineDef).dynamicCast<QGst::Pipeline>();
    }
    catch (const QGlib::Error& ex)
    {
        qCritical() << ex.message();
        qApp->exit(ex.code());

        // Force switch to the main tread
        restart(0);
        return;
    }

    QGst::BusPtr bus = pl->bus();
    QGlib::connect(bus, "message", this, &Mixer::onBusMessage);
    bus->addSignalWatch();
    bus->enableSyncMessageEmission();

    // For all inactive streams bind the handoff handler.
    //
    for (int i = 0; i < inactiveStreams; ++i)
    {
        auto sink = pl->getElementByName(QString("s").append(QString::number(i)).toUtf8());
        QGlib::connect(sink, "handoff", this, &Mixer::onHttpFrame);
    }

    pl->setState(QGst::StatePlaying);
}

void Mixer::onBusMessage(const QGst::MessagePtr& msg)
{
    //qDebug() << msg->typeName() << " " << msg->source()->property("name").toString();
    if (msg->type() == QGst::MessageError)
    {
        auto uri = msg->source()->property("location").toString();
        if (!uri.isEmpty() && srcMap.contains(uri))
        {
            if (srcMap[uri].second)
            {
                qDebug() << uri << "is dead" << msg.staticCast<QGst::ErrorMessage>()->error();
                srcMap[uri].second = false;
                restart(delay);
            }
        }
        else
        {
            qDebug() << msg.staticCast<QGst::ErrorMessage>()->error();
            // Something really bad happend, restarting with a longer delay
            //
            restart(delay * 5);
        }
    }
}

void Mixer::onHttpFrame(const QGst::BufferPtr& b, const QGst::PadPtr& pad)
{
    // Ignore any garbage. The MPEGTS/RTMP frame length is always 4k.
    //
    if (b->size() != 4096)
    {
        return;
    }

    auto uri = pad->peer()->parentElement()->property("location").toString();
    if (!uri.isEmpty() && !srcMap[uri].second)
    {
      qDebug() << uri << "is alive";
      srcMap[uri].second = true;
      restart(delay);
    }
}

void Mixer::timerEvent(QTimerEvent *)
{
    if (updateTimerId)
    {
        killTimer(updateTimerId);
        updateTimerId = 0;
    }

    buildPipeline();
}

void Mixer::onRestart(int delay)
{
    qDebug() << "restarting in" << delay << "msec";
    if (updateTimerId)
    {
        killTimer(updateTimerId);
    }
    updateTimerId = startTimer(delay);
}
