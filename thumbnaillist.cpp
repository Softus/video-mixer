#include "thumbnaillist.h"
#include <QWheelEvent>

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
