#include "clickfilter.h"
#include <QApplication>
#include <QGesture>
#include <QGestureEvent>
#include <QWidget>

ClickFilter::ClickFilter()
{
}

static bool sendLeftMouseEvent(QEvent::Type type, QWidget* widget, const QPoint& pt)
{
    QMouseEvent me(type, widget->mapFromGlobal(pt), pt, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    //QSpontaneKeyEvent::setSpontaneous(&me);
    return qApp->notify(widget, &me);
}

bool ClickFilter::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Gesture)
    {
        auto tapGesture = static_cast<QTapGesture*>(static_cast<QGestureEvent*>(e)->gesture(Qt::TapGesture));
        if (tapGesture && tapGesture->state() == Qt::GestureFinished)
        {
            auto pt = tapGesture->hotSpot().toPoint();
            auto widget = QApplication::widgetAt(pt);
            if (widget)
            {
                return sendLeftMouseEvent(QEvent::MouseButtonPress, widget, pt) &&
                    sendLeftMouseEvent(QEvent::MouseButtonRelease, widget, pt);
            }
        }
    }

    return QObject::eventFilter(o, e);
}
