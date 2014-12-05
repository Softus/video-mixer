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

#ifndef PIPELINE_H
#define PIPELINE_H

#include "videowidget.h"

#include <QDir>
#include <QObject>
#include <QSettings>

#include <QGlib/Error>
#include <QGlib/Value>

#include <QGst/Buffer>
#include <QGst/Element>
#include <QGst/Message>
#include <QGst/Pad>
#include <QGst/Pipeline>

class Pipeline : public QObject
{
    Q_OBJECT
    QString       pipelineDef;

    void onBusMessage(const QGst::MessagePtr& msg);
    void onElementMessage(const QGst::ElementMessagePtr& msg);
    void onImageReady(const QGst::BufferPtr&);
    void onClipFrame(const QGst::BufferPtr&);
    void onVideoFrame(const QGst::BufferPtr&);
    QString buildPipeline(const QSettings& settings, const QString& outputPathDef,
                          bool enableVideoLog, const QString &detectMotion);
    void releasePipeline();
    void errorGlib(const QGlib::ObjectPtr& obj, const QGlib::Error& ex);
    void setElementProperty(const char* elm, const char* prop = nullptr,
                            const QGlib::Value& value = nullptr,
                            QGst::State minimumState = QGst::StatePlaying);
    void setElementProperty(QGst::ElementPtr& elm, const char* prop = nullptr,
                            const QGlib::Value& value = nullptr,
                            QGst::State minimumState = QGst::StatePlaying);

public:
    Pipeline(int index, QObject *parent = 0);
    ~Pipeline();

    bool updatePipeline();
    QString appendVideoTail(const QDir &dir, const QString& prefix, QString clipFileName, bool split);
    void removeVideoTail(const QString& prefix);
    void enableEncoder(bool enable);
    void enableClip(bool enable);
    void enableVideo(bool enable);
    void setImageLocation(const QString& filename);

    QGst::PipelinePtr pipeline;
    QGst::ElementPtr  displaySink;
    QGst::ElementPtr  imageValve;
    QGst::ElementPtr  imageSink;
    QGst::ElementPtr  videoEncoder;
    QGst::ElementPtr  displayOverlay;
    VideoWidget*      displayWidget;

    int           index;
    QString       alias;
    QString       modality;
    bool          motionDetected;
    bool          motionStart;
    bool          motionStop;
    bool          recording;

signals:
    void imageReady();
    void clipFrameReady();
    void videoFrameReady();
    void pipelineError(const QString& text);
    void imageSaved(const QString& filename, const QString& tooltip, const QPixmap& pm);
    void motion(bool detected);

public slots:
    void updateOverlayText(int countdown);

};

#endif // PIPELINE_H
