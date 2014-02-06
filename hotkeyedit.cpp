/*
 * Copyright (C) 2012 Waruna Tennakoon
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

#include "hotkeyedit.h"
#include "mouseshortcut.h"

#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMetaEnum>

HotKeyEdit::HotKeyEdit(QWidget *parent)
    : QLineEdit(parent)
    , m_key(0)
    , m_ignoreNextMoseEvent(false)
{
    updateText();
}

void HotKeyEdit::updateText()
{
    setText(MouseShortcut::toString(m_key));
}

void HotKeyEdit::setKey(int key)
{
    m_key = key;
    updateText();
}

int HotKeyEdit::key() const
{
    return m_key;
}

void HotKeyEdit::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();

    // Handle some special keys
    //
    switch (key)
    {
    case Qt::Key_Escape:
    case Qt::Key_Backspace:
        // Pressing Esc or Backspace will clear the key
        //
        setKey(0);
        emit keyChanged(m_key);
        return;

    // Ignore these keys
    //
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
    case Qt::Key_CapsLock:
    case Qt::Key_NumLock:
    case Qt::Key_ScrollLock:
    case Qt::Key_unknown:
    case -1: // Fake key (f.e. layout change)
        return;

    default:
        break;
    }

    // Checking for key combinations
    //
    key |= event->modifiers();

    setKey(key);
    emit keyChanged(m_key);
}

void HotKeyEdit::handleMousePressEvent(QMouseEvent *evt)
{
    // 'Unknown' mouse buttons still generates the event,
    // but no buttons specified with it. Just ignore them.
    //
    if (!m_ignoreNextMoseEvent && evt->buttons())
    {
        setKey(0x80000000 | evt->modifiers() | evt->buttons());
        emit keyChanged(m_key);
    }
    m_ignoreNextMoseEvent = false;
}

void HotKeyEdit::focusInEvent(QFocusEvent *evt)
{
    switch (evt->reason())
    {
    case Qt::MouseFocusReason:
        m_ignoreNextMoseEvent = true;
        // passthrough
    case Qt::TabFocusReason:
    case Qt::BacktabFocusReason:
    case Qt::ShortcutFocusReason:
        // Display the hint message to the user
        //
        setText(tr("Press a key or a mouse button"));
        break;
    default:
        break;
    }

    QLineEdit::focusInEvent(evt);
}

void HotKeyEdit::focusOutEvent(QFocusEvent *evt)
{
    // Remove the hint message
    //
    updateText();
    QLineEdit::focusOutEvent(evt);
}

bool HotKeyEdit::event(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::MouseButtonPress:
        // Mouse press event must be handled explicitly
        //
        handleMousePressEvent(static_cast<QMouseEvent*>(e));
        // passthrough

    case QEvent::KeyRelease:
    case QEvent::ContextMenu:
    case QEvent::MouseButtonRelease:

        // Local shortcuts like Alt+I will not work while
        // the focus is inside our widget.
        //
    case QEvent::Shortcut:
    case QEvent::ShortcutOverride:
        e->accept();
        return true;
    default:
        return QWidget::event(e);
    }
}
