#ifndef MANDATORYFIELDGROUP_H
#define MANDATORYFIELDGROUP_H

#include <qobject.h>

QT_BEGIN_NAMESPACE
class QPushButton;
class QWidget;
QT_END_NAMESPACE

class MandatoryFieldGroup : public QObject
{
    Q_OBJECT
    void setMandatory(QWidget* widget, bool mandatory);

public:
    MandatoryFieldGroup(QObject *parent)
        : QObject(parent), okButton(0) {}

    void add(QWidget *widget);
    void remove(QWidget *widget);
    void setOkButton(QPushButton *button);

public slots:
    void clear();

private slots:
    void changed();

private:
    QList<QWidget *> widgets;
    QPushButton *okButton;
};

#endif
