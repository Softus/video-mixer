#ifndef WAITCURSOR_H
#define WAITCURSOR_H

#include <QWidget>
#include <QApplication>

class QWaitCursor
{
    QWidget* parent;
public:
#ifndef QT_NO_CURSOR
    QWaitCursor(QWidget* parent, const Qt::CursorShape& shape = Qt::WaitCursor)
        : parent(parent)
    {
        parent->setCursor(shape);
        qApp->processEvents();
    }
    ~QWaitCursor()
    {
        parent->unsetCursor();
        qApp->processEvents();
    }
#else
    QWaitCursor(QWidget*)
    {
    }
#endif
};
#endif
