#include "basewidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <QIcon>

#define DEFAULT_ICON_SIZE 96

BaseWidget::BaseWidget(QWidget *parent) :
    QWidget(parent),
    iconSize(DEFAULT_ICON_SIZE)
{
}

void BaseWidget::error(const QString& msg)
{
    qCritical() << msg;
    QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
}

void BaseWidget::showMaybeMaximized()
{
#ifdef QT_DEBUG
        showNormal();
#else
        showMaximized();
#endif
}

QPushButton* BaseWidget::createButton(const char *slot)
{
    QPushButton* btn = new QPushButton();
    btn->setMinimumSize(QSize(iconSize, iconSize + 8));
    btn->setIconSize(QSize(iconSize, iconSize));
    connect(btn, SIGNAL(clicked()), this, slot);

    return btn;
}

QPushButton* BaseWidget::createButton(const char *icon, const QString &text, const char *slot)
{
    QPushButton* btn = new QPushButton(QIcon(icon), text);
    btn->setMinimumSize(QSize(iconSize, iconSize + 8));
    btn->setIconSize(QSize(iconSize, iconSize));
    connect(btn, SIGNAL(clicked()), this, slot);

    return btn;
}
