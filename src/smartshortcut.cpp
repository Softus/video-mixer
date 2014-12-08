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

#include "smartshortcut.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QMouseEvent>
#include <QSettings>

#define DEFAULT_LONG_PRESS_TIMEOUT   2000

bool grabKey(int key);
bool ungrabKey(int key);

struct SmartTarget
{
    bool global;
    bool longPress;
    QObject *target;
    SmartTarget(bool g, bool l, QObject* t)
        : global(g)
        , longPress(l)
        , target(t)
    {
    }
    SmartTarget(QObject* t)
        : global(false)
        , longPress(false)
        , target(t)
    {
    }

    bool operator == (const SmartTarget& st)
    {
        return target == st.target;
    }
};

static bool isActiveTarget(const SmartTarget& t)
{
    if (t.target->inherits("QAction"))
    {
        auto action = static_cast<QAction*>(t.target);
        if (action->isEnabled())
        {
            auto widget = action->parentWidget();
            if (t.global || !widget || widget->window() == qApp->activeWindow())
            {
                return true;
            }
        }
    }
    else if (t.target->inherits("QAbstractButton"))
    {
        auto btn = static_cast<QAbstractButton*>(t.target);
        if (btn->isEnabled() && (t.global || btn->window() == qApp->activeWindow()))
        {
            return true;
        }
    }

    return false;
}

static bool triggerTarget(bool longPress, const SmartTarget& t)
{
    if (t.longPress != longPress)
    {
        return false;
    }

    if (t.target->inherits("QAction"))
    {
        auto action = static_cast<QAction*>(t.target);
        if (action->isEnabled())
        {
            auto widget = action->parentWidget();
            if (t.global || !widget || widget->window() == qApp->activeWindow())
            {
                action->trigger();
                return true;
            }
        }
    }
    else if (t.target->inherits("QAbstractButton"))
    {
        auto btn = static_cast<QAbstractButton*>(t.target);
        if (btn->isEnabled() && (t.global || btn->window() == qApp->activeWindow()))
        {
            btn->click();
            return true;
        }
    }
    return false;
}

struct SmartHandler
{
    qint64 ts;
    QList<SmartTarget> targets;
    SmartHandler()
        : ts(0)
    {
    }

    bool isActive()
    {
        foreach (auto t, targets)
        {
            if (isActiveTarget(t))
                return true;
        }

        return false;
    }

    bool trigger()
    {
        if (qApp->activeModalWidget())
        {
            qApp->beep();
            return false;
        }

        bool longPress = SmartShortcut::longPressTimeout(ts);

        foreach (auto t, targets)
        {
            if (triggerTarget(longPress, t))
            {
                return true;
            }
        }

        return false;
    }
};

static QHash<int, SmartHandler> handlers;

qint64 SmartShortcut::longPressTimeoutInMsec = 0;

void SmartShortcut::remove(QObject *target)
{
    for (auto h = handlers.begin(); h != handlers.end(); ++h)
    {
        bool global = false;
        auto t = h.value().targets.indexOf(SmartTarget(target));
        if (t >= 0)
        {
           global = h.value().targets[t].global;
           h.value().targets.removeAt(t);
        }

        if (global)
        {
            foreach (auto t, h.value().targets)
            {
                if (t.global)
                {
                    // Found another global key handler,
                    // so keep key grabbed
                    //
                    global = false;
                    break;
                }
            }

            if (global)
            {
                ungrabKey(h.key());
            }
        }

        if (h.value().targets.isEmpty())
        {
            handlers.remove(h.key());
            break;
        }
    }
}

void SmartShortcut::removeAll()
{
    for (auto h = handlers.begin(); h != handlers.end(); ++h)
    {
        foreach (auto t, h.value().targets)
        {
            if (t.global)
            {
                ungrabKey(h.key());
                break;
            }
        }
    }
    handlers.clear();
}

void SmartShortcut::setShortcut(QObject *target, int key)
{
    SmartTarget st(isGlobal(key), isLongPress(key), target);

    auto h = handlers.find(key & ~(GLOBAL_SHORTCUT_MASK | LONG_PRESS_MASK));
    if (h == handlers.end())
    {
        SmartHandler sh;
        sh.targets.append(st);
        handlers.insert(key & ~(GLOBAL_SHORTCUT_MASK | LONG_PRESS_MASK), sh);
    }
    else
    {
        h.value().targets.append(st);
    }

    if (isGlobal(key))
    {
        grabKey(key & ~(GLOBAL_SHORTCUT_MASK | LONG_PRESS_MASK));
    }
}

bool SmartShortcut::isGlobal(int key)
{
    return !!(key & GLOBAL_SHORTCUT_MASK);
}

bool SmartShortcut::isLongPress(int key)
{
    return !!(key & LONG_PRESS_MASK);
}

bool SmartShortcut::isMouse(int key)
{
    return !!(key & MOUSE_SHORTCUT_MASK);
}

QString SmartShortcut::toString(int key, QKeySequence::SequenceFormat format)
{
    key &= ~GLOBAL_SHORTCUT_MASK;

    if (key == 0)
    {
        return tr("(not set)");
    }

    QStringList returnText;

    if (key & LONG_PRESS_MASK)
    {
        returnText += "Long";
        key &= ~LONG_PRESS_MASK;
    }

    // It's a keyboard key, pass to defauls
    //
    if (key > 0)
    {
#if (QT_VERSION < QT_VERSION_CHECK(5, 1, 0))
        // Remove the Keypad modifier due to QTBUG-4022
        //
        key &= ~Qt::KeypadModifier;
#endif
        returnText += QKeySequence(key).toString();
    }
    else
    {
        auto modifiers = key & Qt::MODIFIER_MASK & ~MOUSE_SHORTCUT_MASK;
        if (modifiers)
        {
            auto modifiersStr = QKeySequence(modifiers).toString(format);

            // Turn something like 'CTRL+ALT+' into 'CTRL+ALT'
            //
            modifiersStr.chop(1);

            returnText += modifiersStr;
        }

        auto buttons = key & Qt::MouseButtonMask;

        if (buttons & Qt::LeftButton)    returnText += "LeftButton";
        if (buttons & Qt::RightButton)   returnText += "RightButton";
        if (buttons & Qt::XButton1)      returnText += "BackButton";
        if (buttons & Qt::XButton2)      returnText += "ForwardButton";
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        if (buttons & Qt::MiddleButton)  returnText += "MiddleButton";
        if (buttons & Qt::ExtraButton3)  returnText += "TaskButton";
        if (buttons & Qt::ExtraButton4)  returnText += "ExtraButton4";
        if (buttons & Qt::ExtraButton5)  returnText += "ExtraButton5";
        if (buttons & Qt::ExtraButton6)  returnText += "ExtraButton6";
        if (buttons & Qt::ExtraButton7)  returnText += "ExtraButton7";
        if (buttons & Qt::ExtraButton8)  returnText += "ExtraButton8";
        if (buttons & Qt::ExtraButton9)  returnText += "ExtraButton9";
        if (buttons & Qt::ExtraButton10) returnText += "ExtraButton10";
        if (buttons & Qt::ExtraButton11) returnText += "ExtraButton11";
        if (buttons & Qt::ExtraButton12) returnText += "ExtraButton12";
        if (buttons & Qt::ExtraButton13) returnText += "ExtraButton13";
        if (buttons & Qt::ExtraButton14) returnText += "ExtraButton14";
        if (buttons & Qt::ExtraButton15) returnText += "ExtraButton15";
        if (buttons & Qt::ExtraButton16) returnText += "ExtraButton16";
        if (buttons & Qt::ExtraButton17) returnText += "ExtraButton17";
        if (buttons & Qt::ExtraButton18) returnText += "ExtraButton18";
        if (buttons & Qt::ExtraButton19) returnText += "ExtraButton19";
#else
        if (buttons & Qt::MidButton)     returnText += "MiddleButton";
#endif
    }
    return returnText.join("+");
}

SmartShortcut::SmartShortcut(QObject *parent)
    : QObject(parent)
{
    if (!longPressTimeoutInMsec)
    {
        longPressTimeoutInMsec = QSettings().value("long-press-timeout", DEFAULT_LONG_PRESS_TIMEOUT).toLongLong();
    }

    qApp->installEventFilter(this);
}

SmartShortcut::~SmartShortcut()
{
    qApp->removeEventFilter(this);
}

static bool enabled = true;
void SmartShortcut::setEnabled(bool enable)
{
    enabled = enable;
}

static bool handlePress(int key, QInputEvent* evt)
{
    auto handler = handlers.find(key);
    if (handler != handlers.end())
    {
        if (handler.value().isActive())
        {
            handler.value().ts = SmartShortcut::timestamp();
            evt->accept();
            return true;
        }
    }

    return false;
}

bool SmartShortcut::eventFilter(QObject *o, QEvent *e)
{
    if (enabled)
    {
        switch (e->type())
        {
        case QEvent::KeyPress:
            {
                QKeyEvent *evt = static_cast<QKeyEvent*>(e);
                if (!evt->isAutoRepeat())
                {
                    int key = evt->modifiers() | evt->key();
                    if (handlePress(key, evt))
                    {
                        return true;
                    }
                }
            }
            break;
        case QEvent::MouseButtonPress:
            {
                QMouseEvent *evt = static_cast<QMouseEvent*>(e);
                int btn = MOUSE_SHORTCUT_MASK | evt->modifiers() | evt->button();
                if (handlePress(btn, evt))
                {
                    return true;
                }
            }
            break;
        case QEvent::KeyRelease:
            {
                QKeyEvent *evt = static_cast<QKeyEvent*>(e);
                if (!evt->isAutoRepeat())
                {
                    int key = evt->modifiers() | evt->key();
                    auto handler = handlers.find(key);
                    if (handler != handlers.end())
                    {
                        if (handler.value().trigger())
                        {
                            handler.value().ts = 0;
                        }
                        e->accept();
                        return true;
                    }
                }
            }
            break;
        case QEvent::MouseButtonRelease:
            {
                QMouseEvent *evt = static_cast<QMouseEvent*>(e);
                int btn = MOUSE_SHORTCUT_MASK | evt->modifiers() | evt->button();
                auto handler = handlers.find(btn);
                if (handler != handlers.end())
                {
                    if (handler.value().trigger())
                    {
                        handler.value().ts = 0;
                    }
                    e->accept();
                    return true;
                }
            }
            break;
        default:
            break;
        }
    }

    return QObject::eventFilter(o, e);
}

qint64 SmartShortcut::timestamp()
{
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
    return QDateTime::currentMSecsSinceEpoch();
#else
    return QTime(0,0,0).msecsTo(QTime::currentTime());
#endif
}

bool SmartShortcut::longPressTimeout(qint64 ts)
{
    qDebug() << timestamp() <<  ts << timestamp() - ts;
    return timestamp() - ts > longPressTimeoutInMsec;
}
