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
};

#endif // HORLISTWIDGET_H
