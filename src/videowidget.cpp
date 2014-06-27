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
    swapWith(nullptr);
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
     data->setData("swap-widget", QByteArray());
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
         emit swapWith(drag->target());
     }
}

void VideoWidget::dragEnterEvent(QDragEnterEvent *evt)
{
    if (evt->mimeData()->hasFormat("swap-widget"))
    {
        evt->accept();
    }
}

void VideoWidget::dragMoveEvent(QDragMoveEvent *evt)
{
    if (evt->mimeData()->hasFormat("swap-widget"))
    {
        evt->accept();
    }
}

void VideoWidget::dropEvent(QDropEvent *evt)
{
    if (evt->mimeData()->hasFormat("swap-widget"))
    {
        evt->acceptProposedAction();
    }
}
