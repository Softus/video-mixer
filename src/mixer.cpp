/*
 * Copyright (C) 2015-2018 Softus Inc.
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
#define DEFAULT_RESTART_DELAY 250
#define DEFAULT_PADDING       8
#define DEFAULT_MARGINS       QRect(32, 32, 32, 32)
#define DEFAULT_MESSAGE       "No active sources"

#define DEFAULT_SOURCE  "souphttpsrc is-live=1 do-timestamp=1 location="
#define DEFAULT_SINK    "souphttpclientsink sync=0 location="
#define DEFAULT_DECODER "tsdemux ! avdec_mpeg2video"
#define DEFAULT_ENCODER "avenc_mpeg2video bitrate=1000000 ! mpegtsmux"

Mixer::Mixer(const QString& group, QObject *parent) :
    QObject(parent), group(group), updateTimerId(0)
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

    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        srcMap.insert(settings.value("src").toString(),
            QPair<QString, bool>(settings.value("name").toString(), settings.value("enabled").toBool()));
    }
    settings.endArray();

    if (srcMap.isEmpty())
    {
        qCritical() << "No sources defined";
        return;
    }

    if (dstUri.isEmpty())
    {
        qCritical() << "No destination URI";
        return;
    }

    // This magic required to start/stop timers from the worker threads on Microsoft (R) Windows (TM)
    //
    connect(this, SIGNAL(restart(int)), this, SLOT(onRestart(int)), Qt::QueuedConnection);

    buildPipeline();
}

Mixer::~Mixer()
{
    QSettings settings;
    auto size = settings.beginReadArray(group);
    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        auto url = settings.value("src").toString();
        settings.setValue("enabled", srcMap[url].second);
    }
    settings.endArray();

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

void Mixer::buildPipeline()
{
    int rowSize = rint(ceil(sqrt(srcMap.size())));
    int ypos = (srcMap.size() + rowSize - 1) / rowSize - 1, xpos = (srcMap.size() - 1) % rowSize;
    int inactiveStreams = 0;
    QString pipelineDef;

    // Append output
    //
    pipelineDef
        .append("compositor name=mix background=black ! video/x-raw")
        .append(",width=") .append(QString::number(margins.left() + margins.width()  + width  * rowSize + padding * (rowSize - 1)))
        .append(",height=").append(QString::number(margins.top()  + margins.height() + height * rowSize + padding * (rowSize - 1)))
        .append(" ! ").append(encoder).append(" ! queue ! ").append(sink).append(dstUri).append("\n");

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
                .append(source).append(src.key()).append(" timeout=1")
                .append(" ! queue ! ").append(decoder)
                .append(" ! textoverlay valignment=bottom halignment=right xpad=2 ypad=2 font-desc=32 text=\"")
                .append(text).append("\" ! mix.\n");
        }
        else
        {
            // ...append http source only. And wait for the video stream.
            //
            pipelineDef
                .append(source).append(src.key()).append(" timeout=60 retries=30000")
                .append(" ! fakesink name=s").append(QString::number(inactiveStreams++))
                .append(" sync=0 async=0 signal-handoffs=true\n");
        }
    }

    // No active sources? Add fake one to indicate that mixer itself is alive
    //
    if (inactiveStreams == srcMap.size())
    {
        pipelineDef
            .append("videotestsrc pattern=18 is-live=1 do-timestamp=1 ! video/x-raw")
            .append(",width=") .append(QString::number(margins.left() + margins.width()  + width  * rowSize + padding * (rowSize - 1)))
            .append(",height=").append(QString::number(margins.top()  + margins.height() + height * rowSize + padding * (rowSize - 1)))
            .append(" ! textoverlay halignment=center font-desc=24 text=\"").append(message).append("\" ! mix.\n")
            ;
        restart(10 * 60 * 1000); // Restart every 10 min while in idle mode. Just in case.
    }

    // Prepare the pipeline
    //
    qDebug() << pipelineDef;
    pl = QGst::Parse::launch(pipelineDef).dynamicCast<QGst::Pipeline>();

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

    // Arrange active streams
    //
    if (inactiveStreams < srcMap.size())
    {
        auto mix = pl->getElementByName("mix");
        int activeStreams = 0;

        for (auto src = srcMap.begin(); src != srcMap.end(); ++src)
        {
            // ...if it is still alive...
            //
            if (src.value().second)
            {
                auto sink = mix->getStaticPad(QString("sink_%1").arg(activeStreams++).toUtf8());
                sink->setProperty("width",  width);
                sink->setProperty("height", height);
                sink->setProperty("xpos", margins.left() + (padding + width)  * xpos);
                sink->setProperty("ypos", margins.top()  + (padding + height) * ypos);
            }

            if (--xpos < 0)
            {
                xpos = rowSize - 1;
                --ypos;
            }
        }
    }

    // Finally start the pipeline
    //
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
            qDebug() << group << uri << "is dead" << msg.staticCast<QGst::ErrorMessage>()->error();
            srcMap[uri].second = false;
            restart(delay);
        }
        else
        {
            qDebug() << group << msg.staticCast<QGst::ErrorMessage>()->error();
            // Something really bad happend, restarting with a longer delay
            //
            restart(delay * 10);
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
      qDebug() << group << uri << "is alive";
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

    qApp->exit();
}

void Mixer::onRestart(int delay)
{
    if (!delay)
    {
        qApp->exit(1);
        return;
    }

    qDebug() << group << "restarting in" << delay << "msec";
    if (updateTimerId)
    {
        killTimer(updateTimerId);
    }
    updateTimerId = startTimer(delay);
}
