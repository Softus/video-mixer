#include "thumbnaillist.h"
#include <QWheelEvent>
#include <QKeyEvent>

ThumbnailList::ThumbnailList(QWidget *parent) :
    QListWidget(parent)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setWrapping(false);
}

void ThumbnailList::wheelEvent(QWheelEvent *e)
{
    QWheelEvent horEvent(e->pos(), e->delta(), e->buttons(), e->modifiers(), Qt::Horizontal);
    QListWidget::wheelEvent(&horEvent);
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
