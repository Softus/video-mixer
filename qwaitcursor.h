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

#ifndef WAITCURSOR_H
#define WAITCURSOR_H

#include <QWidget>
#include <QApplication>

class QWaitCursor
{
    QWidget* parent;
public:
#ifndef QT_NO_CURSOR
    QWaitCursor(QWidget* parent, const Qt::CursorShape& shape = Qt::WaitCursor)
        : parent(parent)
    {
        parent->setCursor(shape);
        qApp->processEvents();
    }
    ~QWaitCursor()
    {
        parent->unsetCursor();
        qApp->processEvents();
    }
#else
    QWaitCursor(QWidget*)
    {
    }
#endif
};
#endif
