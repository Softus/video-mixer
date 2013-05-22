#ifndef STUDIESSETTINGS_H
#define STUDIESSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
QT_END_NAMESPACE

class StudiesSettings : public QWidget
{
    Q_OBJECT
    QListWidget* listStudies;
    QPushButton* btnEdit;
    QPushButton* btnRemove;

public:
    Q_INVOKABLE explicit StudiesSettings(QWidget *parent = 0);

signals:

public slots:
    void onAddClicked();
    void onEditClicked();
    void onRemoveClicked();
    void save();
};

#endif // STUDIESSETTINGS_H
