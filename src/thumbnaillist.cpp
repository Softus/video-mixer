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

#include "thumbnaillist.h"
#include <QDrag>
#include <QMimeData>
#include <QWheelEvent>
#include <QKeyEvent>

ThumbnailList::ThumbnailList(bool enableDragRemove, QWidget *parent) :
  QListWidget(parent),
  _enableDragRemove(enableDragRemove)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setWrapping(false);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
}

bool ThumbnailList::enableDragRemove()
{
    return _enableDragRemove;
}

void ThumbnailList::setEnableDragRemove(bool enable)
{
    _enableDragRemove = enable;
}

void ThumbnailList::startDrag(Qt::DropActions /*supportedActions*/)
{
    if (!_enableDragRemove)
    {
        return;
    }

    QListWidgetItem *item = currentItem();

    QDrag *drag = new QDrag(this);
    auto data = new QMimeData();
    data->setData("remove", QByteArray());
    drag->setMimeData(data);

    auto pm = item->icon().pixmap(item->sizeHint());
    drag->setPixmap(pm.copy());
    drag->setHotSpot(pm.rect().center());

    clearSelection();
    if (drag->exec(Qt::MoveAction) == Qt::IgnoreAction)
    {
        emit itemDraggedOut(item);
    }
}

void ThumbnailList::dragEnterEvent(QDragEnterEvent *evt)
{
    if (evt->mimeData()->hasFormat("remove"))
    {
        evt->accept();
    }
    else if (evt->mimeData()->hasFormat("videowidget"))
    {
        evt->setDropAction(Qt::CopyAction);
        evt->accept();
    }
}

void ThumbnailList::dragMoveEvent(QDragMoveEvent *evt)
{
    if (evt->mimeData()->hasFormat("remove"))
    {
        evt->accept();
    }
    else if (evt->mimeData()->hasFormat("videowidget"))
    {
        evt->setDropAction(Qt::CopyAction);
        evt->accept();
    }
}

void ThumbnailList::dropEvent(QDropEvent *evt)
{
    if (evt->mimeData()->hasFormat("videowidget"))
    {
        evt->setDropAction(Qt::CopyAction);
        evt->accept();
    }
}

void ThumbnailList::wheelEvent(QWheelEvent *e)
{
    if (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff)
    {
        QWheelEvent horEvent(e->pos(), e->delta(), e->buttons(), e->modifiers(), Qt::Horizontal);
        QListWidget::wheelEvent(&horEvent);
    }
    else
    {
        QListWidget::wheelEvent(e);
    }
}

void ThumbnailList::keyPressEvent(QKeyEvent *e)
{
    Qt::Key key;
    if (e->key() == Qt::Key_VolumeUp)
    {
        key = Qt::Key_Left;
    }
    else if (e->key() == Qt::Key_VolumeDown)
    {
        key = Qt::Key_Right;
    }
    else
    {
        QListWidget::keyPressEvent(e);
        return;
    }

    QKeyEvent evt(QEvent::KeyPress, key, e->modifiers(), e->text(), e->isAutoRepeat(), e->count());
    QListWidget::keyPressEvent(&evt);
}

void ThumbnailList::keyReleaseEvent(QKeyEvent *e)
{
    Qt::Key key;
    if (e->key() == Qt::Key_VolumeUp)
    {
        key = Qt::Key_Left;
    }
    else if (e->key() == Qt::Key_VolumeDown)
    {
        key = Qt::Key_Right;
    }
    else
    {
        QListWidget::keyReleaseEvent(e);
        return;
    }

    QKeyEvent evt(QEvent::KeyRelease, key, e->modifiers(), e->text(), e->isAutoRepeat(), e->count());
    QListWidget::keyReleaseEvent(&evt);
}
