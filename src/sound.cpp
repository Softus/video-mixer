/*
 * Copyright (C) 2013-2015 Irkutsk Diagnostic Center.
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

#include "sound.h"

#include <QApplication>
#include <QMessageBox>
#include <QUrl>

#include <QGst/ElementFactory>

Sound::Sound(QObject *parent) :
    QObject(parent)
{
}

bool Sound::play(const QString& filePath)
{
    if (!pipeline)
    {
        pipeline = QGst::ElementFactory::make("playbin2").dynamicCast<QGst::Pipeline>();
    }

    if (!pipeline)
    {
        return false;
    }

    pipeline->setState(QGst::StateReady);
    pipeline->getState(nullptr, nullptr, 1000000000L); // 1 sec
    pipeline->setProperty("uri", QUrl::fromLocalFile(filePath).toEncoded());
    auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(pipeline.staticCast<QGst::Bin>(), details, qApp->applicationName().append(".video-edit-preview").toUtf8());
    return QGst::StateChangeFailure != pipeline->setState(QGst::StatePlaying);
}

Sound::~Sound()
{
    if (pipeline)
    {
        pipeline->setState(QGst::StateNull);
        pipeline->getState(nullptr, nullptr, 10000000000L); // 10 sec
        pipeline.clear();
    }
}
