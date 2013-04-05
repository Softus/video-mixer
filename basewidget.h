#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <QWidget>

class QPushButton;

class BaseWidget : public QWidget
{
    Q_OBJECT
protected:
    int          iconSize;
    QPushButton* createButton(const char *slot);
    QPushButton* createButton(const char *icon, const QString &text, const char *slot);

public:
    explicit BaseWidget(QWidget *parent = 0);
    void error(const QString& msg);
    void showMaybeMaximized();

signals:
    
public slots:
    
};

#endif // BASEWIDGET_H
