/*
 * Copyright (C) 2013-2014 Irkutsk Diagnostic Center.
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

#ifndef MOUSESHORTCUT_H
#define MOUSESHORTCUT_H

#include <QObject>
#include <QKeySequence>

QT_BEGIN_NAMESPACE
class QEvent;
class QAbstractButton;
class QAction;
QT_END_NAMESPACE

#define MOUSE_SHORTCUT_MASK  0x80000000
#define GLOBAL_SHORTCUT_MASK 0x00800000

class SmartShortcut : public QObject
{
    Q_OBJECT
    int m_key;
    void init();

public:
    static void remove(QObject *parent);
    static QString toString(int key, QKeySequence::SequenceFormat format = QKeySequence::PortableText);
    static bool isGlobal(int key);
    static bool isMouse(int key);
    QString toString(QKeySequence::SequenceFormat format = QKeySequence::PortableText) const;
    SmartShortcut(int key, QAbstractButton *parent);
    SmartShortcut(int key, QAction *parent);
    ~SmartShortcut();

private slots:
    bool trigger();

protected:
    bool eventFilter(QObject *o, QEvent *e);
};

// T == QAction | QAbstractButton
//
template <class T>
static void updateShortcut(T* btn, int key)
{
    SmartShortcut::remove(btn);
    btn->setShortcut(0);

    if (key == 0)
    {
        btn->setToolTip(btn->text().remove("&"));
        return;
    }

    if (key & (MOUSE_SHORTCUT_MASK | GLOBAL_SHORTCUT_MASK))
    {
        // Will be deleted with parent, or explicitly via removeMouseShortcut
        //
        new SmartShortcut(key, btn);
    }
    else
    {
        btn->setShortcut(QKeySequence(key));
    }
    btn->setToolTip(btn->text().remove("&") + " (" + SmartShortcut::toString(key) + ")");
}

#endif // MOUSESHORTCUT_H
