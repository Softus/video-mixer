#ifndef DICOMSERVERSETTINGS_H
#define DICOMSERVERSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidgetItem;
class QTableWidget;
class QPushButton;
QT_END_NAMESPACE

class DicomServerSettings : public QWidget
{
    Q_OBJECT
    QTableWidget* listServers;
    QPushButton* btnEdit;
    QPushButton* btnRemove;

public:
    Q_INVOKABLE explicit DicomServerSettings(QWidget *parent = 0);

signals:

public slots:
    void onCellChanged(QTableWidgetItem* current, QTableWidgetItem* previous);
    void onCellDoubleClicked(QTableWidgetItem* item);
    void save();
    void onAddClicked();
    void onEditClicked();
    void onRemoveClicked();
};

#endif // DICOMSERVERSETTINGS_H
