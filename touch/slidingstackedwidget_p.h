#ifndef SLIDINGSTACKEDWIDGET_P_H
#define SLIDINGSTACKEDWIDGET_P_H

#include <QObject>
#include <QEasingCurve>

class SlidingStackedWidget;
class QAnimationGroup;

class SlidingStackedWidgetPrivate : public QObject
{
    Q_OBJECT

    SlidingStackedWidget *q_ptr; // q-ptr that points to the API class
    int speed;
    enum QEasingCurve::Type animationtype;
    QWidget*                nextWidget;
    QAnimationGroup*        animationGroup;

    Q_DECLARE_PUBLIC(SlidingStackedWidget)

public:
    SlidingStackedWidgetPrivate(SlidingStackedWidget* q)
        : q_ptr(q)
        , speed(500)
        , animationtype(QEasingCurve::OutBack)
        , nextWidget(nullptr)
        , animationGroup(nullptr)
    {
    }
    void slideInWgt(QWidget* nextWidget, bool rightToLeft);

protected slots:
    //! this is used for internal purposes in the class engine
    void animationDoneSlot();
};

#endif // SLIDINGSTACKEDWIDGET_P_H
