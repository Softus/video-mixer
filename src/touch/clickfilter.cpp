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

#include "clickfilter.h"
#include <QApplication>
#include <QDebug>
#include <QGesture>
#include <QGestureEvent>
#include <QWidget>
#include <QMenu>

ClickFilter::ClickFilter(QObject *parent)
    : QObject(parent)
    , me(QEvent::MouseButtonDblClick, QPoint(), QPoint(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier)
{
    parent->installEventFilter(this);
}

ClickFilter::~ClickFilter()
{
    parent()->removeEventFilter(this);
}

bool ClickFilter::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Gesture)
    {
        auto tapGesture = static_cast<QTapAndHoldGesture*>
                (static_cast<QGestureEvent*>(e)->gesture(Qt::TapAndHoldGesture));

        if (tapGesture && tapGesture->state() == Qt::GestureFinished)
        {
            auto pt = tapGesture->hotSpot().toPoint();
            auto widget = QApplication::widgetAt(pt);

            // For some unknown reason, Qt::GestureFinished comes after QToolButton
            // pops up the menu, so ignore them.
            //
            if (widget && !widget->inherits("QToolButton"))
            {
                auto me = QMouseEvent(QEvent::MouseButtonDblClick, widget->mapFromGlobal(pt)
                    , pt, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                //QSpontaneKeyEvent::setSpontaneous(&me);
                return qApp->notify(widget, &me);
            }
        }
    }

    return QObject::eventFilter(o, e);
}
