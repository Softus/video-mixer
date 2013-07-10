#ifndef MANDATORYFIELDSSETTINGS_H
#define MANDATORYFIELDSSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

class MandatoryFieldsSettings : public QWidget
{
    Q_OBJECT
    QListWidget* listFields;

public:
    Q_INVOKABLE explicit MandatoryFieldsSettings(QWidget *parent = 0);

signals:

public slots:
    void save();
};

#endif // MANDATORYFIELDSSETTINGS_H
