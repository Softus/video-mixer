#ifndef WORKLISTQUERYSETTINGS_H
#define WORKLISTQUERYSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDateEdit;
class QRadioButton;
class QSpinBox;
QT_END_NAMESPACE

class WorklistQuerySettings : public QWidget
{
    Q_OBJECT
    QCheckBox*    checkScheduledDate;
    QRadioButton* radioToday;
    QRadioButton* radioTodayDelta;
    QRadioButton* radioRange;
    QSpinBox*     spinDelta;
    QDateEdit*    dateFrom;
    QDateEdit*    dateTo;
    QCheckBox*    checkModality;
    QComboBox*    cbModality;
    QCheckBox*    checkAeTitle;
    QComboBox*    cbAeTitle;

public:
    Q_INVOKABLE explicit WorklistQuerySettings(QWidget *parent = 0);
    
signals:
    
public slots:
    void save();
};

#endif // WORKLISTQUERYSETTINGS_H
