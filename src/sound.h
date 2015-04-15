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

#ifndef SOUND_H
#define SOUND_H

#include <QObject>

#include <QGst/Pipeline>
#include <gst/gstdebugutils.h>

class Sound : public QObject
{
    Q_OBJECT
public:
    explicit Sound(QObject *parent = 0);
    ~Sound();

    bool play(const QString& filePath);

private:
    QGst::PipelinePtr pipeline;
};

#endif // SOUND_H
