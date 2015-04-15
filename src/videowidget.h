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

#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QGst/Ui/VideoWidget>

class VideoWidget : public QGst::Ui::VideoWidget
{
    Q_OBJECT
    QPoint dragStartPosition;

public:
    explicit VideoWidget(QWidget *parent = 0);

protected:
    virtual void mousePressEvent(QMouseEvent *evt);
    virtual void mouseMoveEvent(QMouseEvent *evt);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void dragEnterEvent(QDragEnterEvent *evt);
    virtual void dragMoveEvent(QDragMoveEvent *e);
    virtual void dropEvent(QDropEvent *e);

signals:
    void swapWith(QWidget* src, QWidget* dst);
    void click();
    void copy();

public slots:
};

#endif // VIDEOWIDGET_H
