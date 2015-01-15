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

#include "videowidget.h"

#include <QApplication>
#include <QDebug>
#include <QDrag>
#include <QDragEnterEvent>
#include <QMimeData>

#include <QGst/Buffer>
#include <QGst/Element>
#include <QGst/Fourcc>
#include <QGst/Structure>

extern QImage ExtractImage(const QGst::BufferPtr& buf, int width = 0);

VideoWidget::VideoWidget(QWidget *parent) :
    QGst::Ui::VideoWidget(parent)
{
    setAcceptDrops(true);
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *evt)
{
    emit click();
    QGst::Ui::VideoWidget::mouseDoubleClickEvent(evt);
}

void VideoWidget::mousePressEvent(QMouseEvent *evt)
{
    if (evt->button() == Qt::LeftButton)
    {
        dragStartPosition = evt->pos();
    }
    QGst::Ui::VideoWidget::mousePressEvent(evt);
}

void VideoWidget::mouseMoveEvent(QMouseEvent *evt)
{
    if (!(evt->buttons() & Qt::LeftButton) ||
        (evt->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance())
    {
        QGst::Ui::VideoWidget::mouseMoveEvent(evt);
        return;
    }

     QDrag *drag = new QDrag(this);
     auto data = new QMimeData();
     data->setData("videowidget", QByteArray());
     drag->setMimeData(data);

     auto sink = videoSink();
     if (sink)
     {
         QImage img;
         auto buffer = sink->property("last-buffer").get<QGst::BufferPtr>();
         if (buffer)
             img = ExtractImage(buffer, 160);

         if (!img.isNull())
         {
             auto pm = QPixmap::fromImage(img);
             drag->setPixmap(pm);
             drag->setHotSpot(pm.rect().center());
         }
     }

     switch (drag->exec(Qt::CopyAction | Qt::MoveAction))
     {
     case Qt::MoveAction:
        if (drag->source() != drag->target())
        {
            emit swapWith(this, static_cast<QWidget*>(drag->target()));
        }
        break;
     case Qt::CopyAction:
     {
         emit copy();
         break;
     }
     default:
         break;
     }
}

void VideoWidget::dragEnterEvent(QDragEnterEvent *evt)
{
    if (evt->mimeData()->hasFormat("videowidget"))
    {
        evt->setDropAction(Qt::MoveAction);
        evt->accept();
    }
}

void VideoWidget::dragMoveEvent(QDragMoveEvent *evt)
{
    if (evt->mimeData()->hasFormat("videowidget"))
    {
        evt->setDropAction(Qt::MoveAction);
        evt->accept();
    }
}

void VideoWidget::dropEvent(QDropEvent *evt)
{
    if (evt->mimeData()->hasFormat("videowidget"))
    {
        evt->setDropAction(Qt::MoveAction);
        evt->accept();
    }
}
