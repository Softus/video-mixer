#ifndef STORAGESETTINGS_H
#define STORAGESETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

class StorageSettings : public QWidget
{
    Q_OBJECT
    QLineEdit* textOutputPath;
    QLineEdit* textFolderTemplate;
    QLineEdit* textImageTemplate;
    QLineEdit* textClipTemplate;
    QLineEdit* textVideoTemplate;

public:
    Q_INVOKABLE explicit StorageSettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void onClickBrowse();
    void save();
};

#endif // STORAGESETTINGS_H
