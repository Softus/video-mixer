#include "mandatoryfieldgroup.h"

// No need for QDateEdit, QSpinBox, etc., since these always return values
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

void MandatoryFieldGroup::add(QWidget *widget)
{
    if (!widgets.contains(widget))
    {
        if (widget->inherits("QCheckBox"))
            connect(widget, SIGNAL(clicked()), this, SLOT(changed()));
        else if (widget->inherits("QComboBox"))
            connect(widget, SIGNAL(highlighted(int)), this, SLOT(changed()));
        else if (widget->inherits("QLineEdit"))
            connect(widget, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
        else
        {
            qWarning("MandatoryFieldGroup: unsupported class %s", widget->metaObject()->className());
            return;
        }
        widgets.append(widget);
        changed();
    }
}

void MandatoryFieldGroup::remove(QWidget *widget)
{
    widget->setBackgroundRole(QPalette::NoRole);
    widgets.removeAll(widget);
    changed();
}


void MandatoryFieldGroup::setOkButton(QPushButton *button)
{
    if (okButton && okButton != button)
    {
        okButton->setEnabled(true);
    }
    okButton = button;
    changed();
}

void MandatoryFieldGroup::setMandatory(QWidget* widget, bool mandatory)
{
    if (widget->property("mandatoryField").toBool() != mandatory)
    {
        widget->setProperty("mandatoryField", mandatory);
        widget->setToolTip(mandatory? tr("This is a mandatory field"): "");
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
    }
}

void MandatoryFieldGroup::changed()
{
    if (!okButton)
        return;

    bool enable = true;
    Q_FOREACH (auto widget, widgets)
    {
        if ((widget->inherits("QCheckBox") && ((QCheckBox*)widget->qt_metacast("QCheckBox"))->checkState() == Qt::PartiallyChecked)
            || (widget->inherits("QComboBox") && ((QComboBox*)widget->qt_metacast("QComboBox"))->currentText().isEmpty())
            || (widget->inherits("QLineEdit") && ((QLineEdit*)widget->qt_metacast("QLineEdit"))->text().isEmpty()))
        {
            enable = false;
            setMandatory(widget, true);
        }
        else
        {
            setMandatory(widget, false);
        }
    }
    okButton->setEnabled(enable);
}

void MandatoryFieldGroup::clear()
{
    Q_FOREACH (auto widget, widgets)
    {
        widget->setProperty("mandatoryField", false);
    }
    widgets.clear();
    if (okButton)
    {
        okButton->setEnabled(true);
        okButton = 0;
    }
}

