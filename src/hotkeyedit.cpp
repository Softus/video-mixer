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
#include "smartshortcut.h"

#include <QDateTime>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMetaEnum>

HotKeyEdit::HotKeyEdit(QWidget *parent)
    : QxtLineEdit(parent)
    , m_key(0)
    , m_ts(0)
    , m_ignoreNextMouseEvent(false)
    , m_stickyKey(0)
{
    updateText();
}

void HotKeyEdit::updateText()
{
    setText(m_key? SmartShortcut::toString(m_key): nullptr);
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

void HotKeyEdit::handleKeyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
    {
        return;
    }

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

    m_stickyKey = key;

    if (SmartShortcut::timestamp() - m_ts > LONG_PRESS_TIMEOUT)
    {
        key |= LONG_PRESS_MASK;
    }

    // Checking for key combinations
    //
    key |= event->modifiers();

    setKey(key);
    emit keyChanged(m_key);
}

void HotKeyEdit::handleMouseReleaseEvent(QMouseEvent *evt)
{
    // 'Unknown' mouse buttons still generates the event,
    // but no buttons specified with it. Just ignore them.
    //
    if (!m_ignoreNextMouseEvent && evt->button() && isEnabled())
    {
        int key = 0x80000000 | evt->modifiers() | evt->button();
        if (SmartShortcut::timestamp() - m_ts > LONG_PRESS_TIMEOUT)
        {
            key |= LONG_PRESS_MASK;
        }
        setKey(key);
        emit keyChanged(m_key);
    }
    m_ignoreNextMouseEvent = false;
}

void HotKeyEdit::focusInEvent(QFocusEvent *evt)
{
    switch (evt->reason())
    {
    case Qt::MouseFocusReason:
        m_ignoreNextMouseEvent = true;
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

    m_stickyKey = 0;
    SmartShortcut::setEnabled(false);
    QLineEdit::focusInEvent(evt);
}

void HotKeyEdit::focusOutEvent(QFocusEvent *evt)
{
    // Remove the hint message
    //
    updateText();
    SmartShortcut::setEnabled(true);
    QLineEdit::focusOutEvent(evt);
}

bool HotKeyEdit::event(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::KeyPress:
        {
            auto evt = static_cast<QKeyEvent*>(e);
            if (!evt->isAutoRepeat() && evt->key() != m_stickyKey)
            {
                m_ts = SmartShortcut::timestamp();
                m_stickyKey = 0;
            }
        }
        e->accept();
        return true;

    case QEvent::MouseButtonPress:
        m_ts = SmartShortcut::timestamp();
        e->accept();
        return true;

    case QEvent::MouseButtonRelease:
        handleMouseReleaseEvent(static_cast<QMouseEvent*>(e));
        e->accept();
        return true;

    case QEvent::KeyRelease:
        handleKeyReleaseEvent(static_cast<QKeyEvent*>(e));
        e->accept();
        return true;

    case QEvent::ContextMenu:

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
