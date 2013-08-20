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
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>

void MandatoryFieldGroup::add(QWidget *widget)
{
    mandatoryFieldColor = QColor(QSettings().value("mandatory-field-color", "pink").toString()).rgba();
    if (!widgets.contains(widget))
    {
        if (widget->inherits("QCheckBox"))
        {
            connect(widget, SIGNAL(clicked()), this, SLOT(changed()));
        }
        else if (widget->inherits("QComboBox"))
        {
            connect(widget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(changed()));
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
    if ((widget->property("mandatoryFieldBaseColor").toUInt() == 0) != mandatory)
    {
        return;
    }

    auto p = QPalette(widget->palette());
    if (mandatory)
    {
        widget->setProperty("mandatoryFieldBaseColor", p.color(QPalette::Base).rgba());
        p.setColor(QPalette::Base, mandatoryFieldColor);
        widget->setToolTip(tr("This is a mandatory field"));
    }
    else
    {
        p.setColor(QPalette::Base, widget->property("mandatoryFieldBaseColor").toUInt());
        widget->setProperty("mandatoryFieldBaseColor", 0);
        widget->setToolTip("");
    }
    widget->setPalette(p);
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

