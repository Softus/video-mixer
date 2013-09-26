#ifndef CLICKFILTER_H
#define CLICKFILTER_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QEvent;
QT_END_NAMESPACE

class ClickFilter : public QObject
{
    Q_OBJECT
public:
    ClickFilter();
protected:
    bool eventFilter(QObject *o, QEvent *e);
};

#endif // CLICKFILTER_H
