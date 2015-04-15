/*
 * Copyright (C) 2013-2015 Irkutsk Diagnostic Center.
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
