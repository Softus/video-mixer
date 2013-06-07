#ifndef WORKLISTCOLUMNSETTINGS_H
#define WORKLISTCOLUMNSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

class WorklistColumnSettings : public QWidget
{
    Q_OBJECT
    QTableWidget* listColumns;
    QStringList  checkedColumns();

public:
    Q_INVOKABLE explicit WorklistColumnSettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void save();
};

#endif // WORKLISTCOLUMNSETTINGS_H
