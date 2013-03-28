#include "basewidget.h"
#include <QDebug>
#include <QMessageBox>

BaseWidget::BaseWidget(QWidget *parent) :
    QWidget(parent)
{
}

void BaseWidget::error(const QString& msg)
{
    qCritical() << msg;
    QMessageBox(QMessageBox::Critical, windowTitle(), msg, QMessageBox::Ok, this).exec();
}
