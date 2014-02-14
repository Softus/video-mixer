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

#ifndef HORLISTWIDGET_H
#define HORLISTWIDGET_H

#include <QListWidget>

class ThumbnailList : public QListWidget
{
    Q_OBJECT
public:
    explicit ThumbnailList(QWidget *parent = 0);

protected:
    void wheelEvent(QWheelEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
};

#endif // HORLISTWIDGET_H
