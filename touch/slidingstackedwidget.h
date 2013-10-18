#ifndef SLIDINGSTACKEDWIDGET_H
#define SLIDINGSTACKEDWIDGET_H

#include <QStackedWidget>
#include <QEasingCurve>

QT_BEGIN_NAMESPACE
class QGestureEvent;
class QAnimationGroup;
QT_END_NAMESPACE
class SlidingStackedWidgetPrivate;

/*!
Description
        SlidingStackedWidget is a class that is derived from QtStackedWidget
        and allows smooth side shifting of widgets, in addition
        to the original hard switching from one to another as offered by
        QStackedWidget itself.
*/

class SlidingStackedWidget: public QStackedWidget
{
    Q_OBJECT

    Q_PROPERTY(QEasingCurve::Type animation READ animation WRITE setAnimation NOTIFY animationChanged)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY speedChanged)

public:
    SlidingStackedWidget(QWidget *parent = nullptr);
    ~SlidingStackedWidget() = default;

protected:
    virtual bool event(QEvent *e);

public slots:
    //! Some basic settings API
    int speed() const;
    QEasingCurve::Type animation() const;
    void setAnimation(QEasingCurve::Type animationtype);
    void setSpeed(int speed);

    //! The Animation / Page Change API
    void slideInNext();
    void slideInPrev();
    void slideInWidget(QWidget* widget);
    void slideInIdx(int idx);

Q_SIGNALS:
    void slideInDone();
    void speedChanged(int speed);
    void animationChanged(QEasingCurve::Type type);

private:
    Q_DISABLE_COPY(SlidingStackedWidget)
    Q_DECLARE_PRIVATE(SlidingStackedWidget)

protected:
    //! this is used for internal purposes in the class engine
    bool gestureEvent(QGestureEvent *e);
    SlidingStackedWidgetPrivate* d_ptr;
};

#endif // SLIDINGSTACKEDWIDGET_H
