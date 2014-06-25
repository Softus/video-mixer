#include "videowidget.h"

#include <QDebug>
#include <QDrag>
#include <QDragEnterEvent>
#include <QMimeData>

#include <QGst/Buffer>
#include <QGst/Element>
#include <QGst/Fourcc>
#include <QGst/Structure>

extern QImage ExtractImage(const QGst::BufferPtr& buf, int width = 0);

VideoWidget::VideoWidget(int index, QWidget *parent) :
    QGst::Ui::VideoWidget(parent)
{
    setAcceptDrops(true);
}

void VideoWidget::mousePressEvent(QMouseEvent *)
{
     QDrag *drag = new QDrag(this);
     auto data = new QMimeData();
     data->setData("set-active-source", QByteArray());
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

     if (Qt::MoveAction == drag->exec(Qt::MoveAction) && drag->source() != drag->target())
     {
         qDebug() << "swap" <<drag->source() << this << drag->target();
         emit swap(1,0);//index, static_cast<VideoWidget*>(drag->target())->index);
     }
}

void VideoWidget::dragEnterEvent(QDragEnterEvent *evt)
{
    if (evt->mimeData()->hasFormat("set-active-source"))
    {
        evt->accept();
        return;
    }
    QGst::Ui::VideoWidget::dragEnterEvent(evt);
}

void VideoWidget::dragMoveEvent(QDragMoveEvent *evt)
{
    if (evt->mimeData()->hasFormat("set-active-source"))
    {
        evt->accept();
        return;
    }
    QGst::Ui::VideoWidget::dragMoveEvent(evt);
}

void VideoWidget::dropEvent(QDropEvent *evt)
{
    if (evt->mimeData()->hasFormat("set-active-source"))
    {
        evt->accept();
        return;
    }
    QGst::Ui::VideoWidget::dropEvent(evt);
}
