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
    void swapWith(QWidget* w);

public slots:
};

#endif // VIDEOWIDGET_H
