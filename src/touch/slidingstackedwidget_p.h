/*
 * Copyright (C) 2013-2014 Irkutsk Diagnostic Center.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
