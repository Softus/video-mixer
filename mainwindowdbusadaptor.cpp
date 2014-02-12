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

#include "mainwindowdbusadaptor.h"
#include "mainwindow.h"

#include <QTimer>
#include <QDebug>

MainWindowDBusAdaptor::MainWindowDBusAdaptor(MainWindow *wnd)
    : QDBusAbstractAdaptor(wnd), wnd(wnd)
{
}

bool MainWindowDBusAdaptor::busy()
{
    return wnd->running;
}

bool MainWindowDBusAdaptor::startStudy
    ( const QString &id, const QString &name
    , const QString &sex, const QString &birthdate
    , const QString &physician, const QString &studyName
    , bool autoStart
    )
{
    if (!id.isEmpty())
        wnd->patientId = id;
    if (!name.isEmpty())
        wnd->patientName = name;
    if (!sex.isEmpty())
        wnd->patientSex = sex;
    if (!birthdate.isEmpty())
        wnd->patientBirthDate = birthdate;
    if (!physician.isEmpty())
        wnd->physician = physician;
    if (!studyName.isEmpty())
        wnd->studyName = studyName;

    auto state = wnd->windowState();
    wnd->setWindowState(state | Qt::WindowFullScreen);
    wnd->activateWindow();
    wnd->setWindowState(state);

    if (autoStart)
    {
        wnd->onStartStudy();
    }

    return true;
}
