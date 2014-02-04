/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
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

#include "typedetect.h"

#include <QGst/ElementFactory>
#include <QGst/Pipeline>
#include <QGst/Structure>

QString TypeDetect(const QString& filePath)
{
    QGst::State   state;
    QGst::CapsPtr caps;
    auto pipeline = QGst::Pipeline::create("typedetect");
    auto source   = QGst::ElementFactory::make("filesrc", "source");
    auto typefind = QGst::ElementFactory::make("typefind", "typefind");
    auto fakesink = QGst::ElementFactory::make("fakesink", "fakesink");

    if (pipeline && source && typefind && fakesink)
    {
        pipeline->add(source, typefind, fakesink);
        QGst::Element::linkMany(source, typefind, fakesink);

        source->setProperty("location", filePath);
        pipeline->setState(QGst::StatePaused);
        if (QGst::StateChangeSuccess == pipeline->getState(&state, nullptr, -1))
        {
            auto prop = typefind->property("caps");
            if (prop)
            {
                caps = prop.get<QGst::CapsPtr>();
            }
        }
        pipeline->setState(QGst::StateNull);
    }

    if (caps)
    {
        auto str = caps->internalStructure(0);
        if (str)
        {
            return str->name();
        }
    }

    return nullptr;
}
