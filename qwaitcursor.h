#ifndef WAITCURSOR_H
#define WAITCURSOR_H

#include <QWidget>

class QWaitCursor
{
    QWidget* parent;
public:
#ifndef QT_NO_CURSOR
    QWaitCursor(QWidget* parent, const Qt::CursorShape& shape = Qt::WaitCursor)
        : parent(parent)
    {
        parent->setCursor(shape);
    }
    ~QWaitCursor()
    {
        parent->unsetCursor();
    }
#else
    QWaitCursor(QWidget*)
    {
    }
#endif
};
#endif
