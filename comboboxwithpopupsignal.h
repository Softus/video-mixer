#ifndef COMBOBOXWITHPOPUPSIGNAL_H
#define COMBOBOXWITHPOPUPSIGNAL_H

#include <QComboBox>

class ComboBoxWithPopupSignal : public QComboBox
{
    Q_OBJECT
public:

    virtual void showPopup()
    {
        beforePopup();
        QComboBox::showPopup();
    }

Q_SIGNALS:
    void beforePopup();
};

#endif // COMBOBOXWITHPOPUPSIGNAL_H
