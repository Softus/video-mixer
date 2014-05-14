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

#include <QDebug>
#include <QVector>
#include <X11/Xlib.h>
#include <X11/Xproto.h>

#if QT_VERSION < QT_VERSION_CHECK(5,0,0) || QT_VERSION >= QT_VERSION_CHECK(5,1,0)
#include <QX11Info>
#else
class QX11Info
{
public:
    static Display *display()
    {
        return XOpenDisplay(0);
    }

    static Qt::HANDLE appRootWindow()
    {
        return DefaultRootWindow(display());
    }
};
#endif

struct X11ErrorHandler
{
    static bool error;

    static int X11ErrorHandlerCallback(Display *display, XErrorEvent *event)
    {
        switch (event->request_code)
        {
            case X_GrabButton:
            case X_GrabKey:
            case X_UngrabButton:
            case X_UngrabKey:
            {
                error = true;
                char errstr[256];
                XGetErrorText(display, event->error_code, errstr, 256);
                qDebug() << errstr;
            }
            break;
        }
        return 0;
    }

    X11ErrorHandler()
    {
        error = false;
        m_previousErrorHandler = XSetErrorHandler(X11ErrorHandlerCallback);
    }

    ~X11ErrorHandler()
    {
        XSetErrorHandler(m_previousErrorHandler);
    }

private:
    XErrorHandler m_previousErrorHandler;
};

bool X11ErrorHandler::error = false;

const QVector<quint32> maskModifiers = QVector<quint32>()
    << 0 << Mod2Mask << LockMask << (Mod2Mask | LockMask);

static QList<unsigned int> translateButtons(int buttons)
{
    QList<unsigned int> ret;
    if (buttons & Qt::LeftButton)    ret << Button1;
    if (buttons & Qt::MidButton)     ret << Button2;
    if (buttons & Qt::RightButton)   ret << Button3;
    if (buttons & Qt::XButton1)      ret << Button4;
    if (buttons & Qt::XButton2)      ret << Button5;

    return ret;
}

static unsigned int translateModifiers(int modifiers)
{
    // ShiftMask, LockMask, ControlMask, Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, and Mod5Mask
    auto native = 0;
    if (modifiers & Qt::ShiftModifier)
        native |= ShiftMask;
    if (modifiers & Qt::ControlModifier)
        native |= ControlMask;
    if (modifiers & Qt::AltModifier)
        native |= Mod1Mask;
    if (modifiers & Qt::MetaModifier)
        native |= Mod4Mask;

    return native;
}

static unsigned int translateKey(int key, Display* display)
{
    KeySym keysym = XStringToKeysym(QKeySequence(key).toString().toLatin1().data());
    if (keysym == NoSymbol)
        keysym = static_cast<ushort>(key);
    return XKeysymToKeycode(display, keysym);
}

bool ungrabKey(int key)
{
    X11ErrorHandler errorHandler;
    auto display = QX11Info::display();
    auto appWindow = QX11Info::appRootWindow();
    auto mod = translateModifiers(key);

    if (MOUSE_SHORTCUT_MASK & key)
    {
        Q_FOREACH (auto btn, translateButtons(key))
        {
            for (int i = 0; !errorHandler.error && i < maskModifiers.size(); ++i)
            {
                XUngrabButton(display, btn, maskModifiers[i] | mod, appWindow);
            }
        }
    }
    else
    {
        auto xkey = translateKey(key, display);
        for (int i = 0; !errorHandler.error && i < maskModifiers.size(); ++i)
        {
            XUngrabKey(display, xkey, maskModifiers[i] | mod, appWindow);
        }
    }

    return !errorHandler.error;
}

bool grabKey(int key)
{
    X11ErrorHandler errorHandler;
    auto display = QX11Info::display();
    auto appWindow = QX11Info::appRootWindow();
    auto mod = translateModifiers(key);

    if (MOUSE_SHORTCUT_MASK & key)
    {
        Q_FOREACH (auto btn, translateButtons(key))
        {
            for (int i = 0; !errorHandler.error && i < maskModifiers.size(); ++i)
            {
                XGrabButton(display, btn, maskModifiers[i] | mod, appWindow, True,
                            ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
            }
        }
    }
    else
    {
        auto xkey = translateKey(key, display);
        for (int i = 0; !errorHandler.error && i < maskModifiers.size(); ++i)
        {
            XGrabKey(display, xkey, maskModifiers[i] | mod, appWindow, True,
                        GrabModeAsync, GrabModeAsync);
        }
    }

    if (errorHandler.error)
    {
        ungrabKey(key);
        return false;
    }
    return true;
}
