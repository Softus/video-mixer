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

#include "mouseshortcut.h"

#include <QApplication>
#include <QMouseEvent>
#include <QAbstractButton>
#include <QAction>

void MouseShortcut::removeMouseShortcut(QObject *parent)
{
    // Remove any existent mouse shortcut
    //
    Q_FOREACH(auto ch, parent->children())
    {
        if (ch->inherits("MouseShortcut"))
        {
            delete(ch);
        }
    }
}

QString MouseShortcut::toString(int key, QKeySequence::SequenceFormat format)
{
    if (key == 0)
    {
        return tr("(not set)");
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
        return QKeySequence(key).toString();
    }

    QStringList returnText;

    auto modifiers = key & Qt::MODIFIER_MASK & ~0x80000000;
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
    if (buttons & Qt::ExtraButton20) returnText += "ExtraButton20";
    if (buttons & Qt::ExtraButton21) returnText += "ExtraButton21";
    if (buttons & Qt::ExtraButton22) returnText += "ExtraButton22";
    if (buttons & Qt::ExtraButton23) returnText += "ExtraButton23";
    if (buttons & Qt::ExtraButton24) returnText += "ExtraButton24";
#else
    if (buttons & Qt::MidButton)     returnText += "MiddleButton";
#endif
    return returnText.join("+");
}

QString MouseShortcut::toString(QKeySequence::SequenceFormat format) const
{
    return toString(m_key, format);
}

MouseShortcut::MouseShortcut(int key, QAbstractButton *parent) :
    QObject(parent), m_key(key)
{
    qApp->installEventFilter(this);
}

MouseShortcut::MouseShortcut(int key, QAction *parent) :
    QObject(parent), m_key(key)
{
    qApp->installEventFilter(this);
}

MouseShortcut::~MouseShortcut()
{
    qApp->removeEventFilter(this);
}

bool MouseShortcut::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *evt = static_cast<QMouseEvent*>(e);
        int btn = 0x80000000 | evt->modifiers() | evt->buttons();

        if (m_key == btn)
        {
            if (parent()->inherits("QAction"))
            {
                auto action = static_cast<QAction*>(parent());
                auto widget = action->parentWidget();
                if (!widget || widget->window() == qApp->activeWindow())
                {
                    action->trigger();
                    e->accept();
                    return true;
                }
            }
            else if (parent()->inherits("QAbstractButton"))
            {
                auto btn = static_cast<QAbstractButton*>(parent());
                if (btn->window() == qApp->activeWindow())
                {
                    btn->click();
                    e->accept();
                    return true;
                }
            }
        }
    }

    return QObject::eventFilter(o, e);
}
