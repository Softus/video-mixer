#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QGst/Ui/VideoWidget>

class VideoWidget : public QGst::Ui::VideoWidget
{
    Q_OBJECT

public:
    VideoWidget(int index, QWidget *parent = 0);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *evt);
    virtual void dragMoveEvent(QDragMoveEvent *e);
    virtual void dropEvent(QDropEvent *e);

signals:
    void swap(int src, int dst);

public slots:
};

#endif // VIDEOWIDGET_H
