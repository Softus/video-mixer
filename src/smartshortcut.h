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

#ifndef SMARTSHORTCUT_H
#define SMARTSHORTCUT_H

#include <QObject>
#include <QKeySequence>

QT_BEGIN_NAMESPACE
class QEvent;
class QAbstractButton;
class QAction;
class QInputEvent;
QT_END_NAMESPACE

#define MOUSE_SHORTCUT_MASK  0x80000000
#define GLOBAL_SHORTCUT_MASK 0x00800000
#define LONG_PRESS_MASK      0x00400000

class SmartShortcut : public QObject
{
    Q_OBJECT
    static qint64 longPressTimeoutInMsec;

public:
    static qint64 timestamp();
    static bool longPressTimeout(qint64 ts);
    static void remove(QObject *parent);
    static void removeAll();
    static void setShortcut(QObject *parent, int key);
    static void setEnabled(bool enable);
    static QString toString(int key, QKeySequence::SequenceFormat format = QKeySequence::PortableText);
    static bool isGlobal(int key);
    static bool isLongPress(int key);
    static bool isMouse(int key);
    SmartShortcut(QObject* parent);
    ~SmartShortcut();

protected:
    bool eventFilter(QObject *o, QEvent *e);
};

// T == QAction | QAbstractButton
//
template <class T>
static void updateShortcut(T* btn, int key)
{
    SmartShortcut::remove(btn);

    if (key == 0)
    {
        btn->setToolTip(btn->text().remove("&"));
    }
    else
    {
        SmartShortcut::setShortcut(btn, key);
        btn->setToolTip(btn->text().remove("&") + " (" + SmartShortcut::toString(key) + ")");
    }
}

#endif // SMARTSHORTCUT_H
