/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mandatoryfieldgroup.h"

// No need for QDateEdit, QSpinBox, etc., since these always return values
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSettings>
#include <QxtLineEdit>

MandatoryFieldGroup::MandatoryFieldGroup(QObject *parent)
    : QObject(parent), okButton(nullptr)
{
    mandatoryFieldColor = QColor(QSettings().value("mandatory-field-color", "red").toString()).rgba();
}

void MandatoryFieldGroup::add(QWidget *widget)
{
    // For comboboxes, datepicker and like that we shuld use inner text edit widget
    //
    if (widget->inherits("QComboBox"))
    {
        widget = ((QComboBox*)widget->qt_metacast("QComboBox"))->lineEdit();
    }

    if (widgets.contains(widget))
    {
        return;
    }

    if (widget->inherits("QCheckBox"))
    {
        connect(widget, SIGNAL(clicked()), this, SLOT(changed()));
    }
    else if (widget->inherits("QLineEdit"))
    {
        connect(widget, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
    }
    else
    {
        qWarning("MandatoryFieldGroup: unsupported class %s", widget->metaObject()->className());
        return;
    }
    widgets.append(widget);
    changed();
}

void MandatoryFieldGroup::remove(QWidget *widget)
{
    if (widget->inherits("QComboBox"))
    {
        widget = ((QComboBox*)widget->qt_metacast("QComboBox"))->lineEdit();
    }

    if (widgets.removeAll(widget))
    {
        widget->setBackgroundRole(QPalette::NoRole);
        changed();
    }
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
    if ((widget->property("mandatoryFieldBaseColor").toUInt() == 0) != mandatory)
    {
        return;
    }

    QWidget* label = nullptr;
    if (parent())
    {
        auto layoutParent = static_cast<QWidget*>(parent())->layout();
        if (layoutParent && layoutParent->inherits("QFormLayout"))
        {
            auto layoutForm = ((QFormLayout*)layoutParent->qt_metacast("QFormLayout"));
            label = layoutForm->labelForField(widget);
            if (!label)
            {
                // For line edit in a combobox, datepicker and like that we shuld use parent label
                //
                label = layoutForm->labelForField(static_cast<QWidget*>(widget->parent()));
            }
        }
    }

    if (!label)
    {
        // Checkboxes usually has no label.
        //
        label = widget;
    }

    auto p = QPalette(label->palette());
    if (mandatory)
    {
        widget->setProperty("mandatoryFieldBaseColor", p.color(QPalette::Foreground).rgba());
        p.setColor(QPalette::Foreground, mandatoryFieldColor);
        widget->setToolTip(tr("This is a mandatory field"));
        if (widget->inherits("QxtLineEdit"))
        {
            ((QxtLineEdit*)widget->qt_metacast("QxtLineEdit"))->setSampleText(tr("This is a mandatory field"));
        }
    }
    else
    {
        p.setColor(QPalette::Foreground, widget->property("mandatoryFieldBaseColor").toUInt());
        widget->setProperty("mandatoryFieldBaseColor", 0);
        widget->setToolTip("");
        if (widget->inherits("QxtLineEdit"))
        {
            ((QxtLineEdit*)widget->qt_metacast("QxtLineEdit"))->setSampleText("");
        }
    }
    label->setPalette(p);
}

void MandatoryFieldGroup::changed()
{
    if (!okButton)
        return;

    bool enable = true;
    Q_FOREACH (auto widget, widgets)
    {
        if ((widget->inherits("QCheckBox") && ((QCheckBox*)widget->qt_metacast("QCheckBox"))->checkState() == Qt::PartiallyChecked)
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
        okButton = nullptr;
    }
}

