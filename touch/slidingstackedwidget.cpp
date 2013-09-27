#include "slidingstackedwidget.h"
#include "slidingstackedwidget_p.h"

#include <QGestureEvent>
#include <QPanGesture>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

SlidingStackedWidget::SlidingStackedWidget(QWidget *parent)
    : QStackedWidget(parent)
    , d_ptr(new SlidingStackedWidgetPrivate(this))
{
    grabGesture(Qt::PanGesture, Qt::IgnoredGesturesPropagateToParent);
}

SlidingStackedWidget::~SlidingStackedWidget()
{
}

bool SlidingStackedWidget::event(QEvent *e)
{
    if (e->type() == QEvent::Gesture)
    {
        return gestureEvent(static_cast<QGestureEvent*>(e));
    }
    return QStackedWidget::event(e);
}

bool SlidingStackedWidget::gestureEvent(QGestureEvent *e)
{
    enum { MIN_LENGTH = 100 };

    auto panGesture = static_cast<QPanGesture*>(e->gesture(Qt::PanGesture));
    if (panGesture && panGesture->state() == Qt::GestureFinished)
    {
        auto length = panGesture->offset().x();
        if (length > MIN_LENGTH)
        {
            slideInPrev();
        }
        else if (length < -MIN_LENGTH)
        {
            slideInNext();
        }
        e->accept(Qt::PanGesture);
        return true;
    }

    return QStackedWidget::event(e);
}

int SlidingStackedWidget::speed() const
{
    return d_func()->speed;
}

void SlidingStackedWidget::setSpeed(int speed)
{
    Q_D(SlidingStackedWidget);
    d->speed = speed;
    emit speedChanged(speed);
}

QEasingCurve::Type SlidingStackedWidget::animation() const
{
    return d_func()->animationtype;
}

void SlidingStackedWidget::setAnimation(QEasingCurve::Type animationtype)
{
    Q_D(SlidingStackedWidget);
    d->animationtype = animationtype;
    emit animationChanged(animationtype);
}

void SlidingStackedWidget::slideInNext()
{
    slideInIdx(currentIndex() + 1);
}

void SlidingStackedWidget::slideInPrev()
{
    slideInIdx(currentIndex() - 1);
}

void SlidingStackedWidget::slideInWidget(QWidget* widget)
{
    slideInIdx(indexOf(widget));
}

void SlidingStackedWidget::slideInIdx(int idx)
{
    Q_D(SlidingStackedWidget);
    bool rightToLeft = idx > currentIndex();

    if (idx >= count())
    {
        idx = 0;
    }
    else if (idx < 0)
    {
        idx = count() - 1;
    }
    d->slideInWgt(widget(idx), rightToLeft);
}

void SlidingStackedWidgetPrivate::slideInWgt(QWidget* next, bool rightToLeft)
{
    Q_Q(SlidingStackedWidget);
    auto curr = q->currentWidget();
    if (next == curr)
    {
        return;
    }

    if (animationGroup)
    {
        // At the moment, do not allow re-entrance before an animation is completed.
        // other possibility may be to finish the previous animation abrupt, or
        // to revert the previous animation with a counter animation, before going ahead
        // or to revert the previous animation abrupt
        //
        return;
    }

    // The following is important, to ensure that the new widget
    // has correct geometry information when sliding in first time
    //
    next->setGeometry(q->frameRect());

    int offsetx = q->width();
    if (rightToLeft)
    {
        offsetx = -offsetx;
    }

    auto currPos = curr->pos();
    auto nextPos = next->pos();

    // Re-position the next widget outside/aside of the display area
    //
    next->move(nextPos.x() - offsetx, nextPos.y());

    // Make it visible/show
    //
    next->show();
    next->raise();

    // Animate both, the now and next widget to the side, using animation framework
    //
    auto animCurr = new QPropertyAnimation(curr , "pos");
    animCurr->setDuration(speed);
    animCurr->setEasingCurve(animationtype);
    animCurr->setStartValue(currPos);
    animCurr->setEndValue(QPoint(currPos.x() + offsetx, currPos.y()));

    auto animNext = new QPropertyAnimation(next, "pos");
    animNext->setDuration(speed);
    animNext->setEasingCurve(animationtype);
    animNext->setStartValue(QPoint(nextPos.x() - offsetx, nextPos.y()));
    animNext->setEndValue(nextPos);

    animationGroup = new QParallelAnimationGroup;
    animationGroup->addAnimation(animCurr);
    animationGroup->addAnimation(animNext);

    // The rest is done via a connect from the animation ready;
    // animation->finished() provides a signal when animation is done;
    // so we connect this to some post processing slot,
    // that we implement here below in animationDoneSlot.
    //
    QObject::connect(animationGroup, SIGNAL(finished()), this, SLOT(animationDoneSlot()));
    nextWidget = next;
    animationGroup->start();
}

void SlidingStackedWidgetPrivate::animationDoneSlot(void)
{
    Q_Q(SlidingStackedWidget);
    auto currWidget = q->currentWidget();

    // When ready, call the QStackedWidget slot setCurrentIndex(int)
    //
    q->setCurrentWidget(nextWidget);  //this function is inherit from QStackedWidget

    // then hide the outshifted widget now, and  (may be done already implicitely by QStackedWidget)
    //
    currWidget->hide();

    // then set the position of the outshifted widget now back to its original
    //
    currWidget->setGeometry(q->frameRect());

    delete animationGroup;
    animationGroup = nullptr;
    emit q->slideInDone();
}
