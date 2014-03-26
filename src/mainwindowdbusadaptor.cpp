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
#include <QSettings>

MainWindowDBusAdaptor::MainWindowDBusAdaptor(MainWindow *wnd)
    : QDBusAbstractAdaptor(wnd), wnd(wnd)
{
}

bool MainWindowDBusAdaptor::busy()
{
    return wnd->running;
}

bool MainWindowDBusAdaptor::startStudy
    ( const QString &accessionNumber
    , const QString &id, const QString &name
    , const QString &sex, const QString &birthdate
    , const QString &physician, const QString &studyName
    , bool autoStart
    )
{
    if (busy())
    {
        if (accessionNumber == wnd->accessionNumber && id == wnd->patientId)
        {
            // Duplicate study, just ignore
            //
            return false;
        }
        else
        {
            // The user forgot to stop previous study => stop right now
            //
            wnd->onStopStudy();
        }
    }

    if (!accessionNumber.isEmpty())
        wnd->accessionNumber = accessionNumber;
    if (!id.isEmpty())
        wnd->patientId = id;
    if (!name.isEmpty())
        wnd->patientName = name;
    if (!sex.isEmpty())
        wnd->patientSex = sex;
    if (!birthdate.isEmpty())
        wnd->patientBirthDate = QString(birthdate).remove(QRegExp("[^0-9]"));
    if (!physician.isEmpty())
        wnd->physician = physician;
    if (!studyName.isEmpty())
        wnd->studyName = studyName;

    if (wnd->isMinimized())
        wnd->showNormal();
    if (wnd->isHidden())
        wnd->show();

    wnd->activateWindow();

    if (autoStart)
    {
        QTimer::singleShot(0, wnd, SLOT(onStartClick()));
    }

    return true;
}

QString MainWindowDBusAdaptor::value
    ( const QString &name
    )
{
    return QSettings().value(name).toString();
}

void MainWindowDBusAdaptor::setValue
    ( const QString &name
    , const QString &value
    )
{
    QSettings().setValue(name, value);
}

void MainWindowDBusAdaptor::setConfigPath
    ( const QString &path
    )
{
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, path);
}

