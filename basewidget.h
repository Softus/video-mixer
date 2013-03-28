#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <QWidget>

class BaseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BaseWidget(QWidget *parent = 0);
    void error(const QString& msg);

signals:
    
public slots:
    
};

#endif // BASEWIDGET_H
