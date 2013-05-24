#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
QT_END_NAMESPACE

class Settings : public QDialog
{
    Q_OBJECT
    QPushButton *btnCancel;
public:
    explicit Settings(QWidget *parent = 0, Qt::WindowFlags flags = 0);

signals:
    void save();
    void apply();

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
    void onClickApply();

    virtual void accept();

private:
    void createPages();
    void createPage(const QString& title, const QMetaObject& page);

    QListWidget *listWidget;
    QStackedWidget *pagesWidget;
};

#endif
