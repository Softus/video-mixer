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
#include "pipeline.h"

#include <QTimer>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDebug>
#include <QSettings>

MainWindowDBusAdaptor::MainWindowDBusAdaptor(MainWindow *wnd)
    : QDBusAbstractAdaptor(wnd), wnd(wnd)
{
}

bool MainWindowDBusAdaptor::connectToService(bool systemBus, const QString &service, const QString &path, const QString &interface)
{
    auto ok = true;
    auto bus = systemBus? QDBusConnection::systemBus(): QDBusConnection::sessionBus();

    // Activate the service
    //
    new QDBusInterface(service, path, interface, bus, this);

    ok = ok && (bus.connect(service, path, interface, "takeSnapshot", this, SLOT(takeSnapshot()))
             || bus.connect(service, path, interface, "takeSnapshot", "string", this, SLOT(takeSnapshot(String))));
    ok = ok && (bus.connect(service, path, interface, "startRecord",  this, SLOT(startRecord()))
             || bus.connect(service, path, interface, "startRecord", "int32",  this, SLOT(startRecord(int)))
             || bus.connect(service, path, interface, "startRecord", "int32,string",  this, SLOT(startRecord(int,String))));
    ok = ok && (bus.connect(service, path, interface, "stopRecord",   this, SLOT(stopRecord())));

    return ok;
}

bool MainWindowDBusAdaptor::busy()
{
    return wnd->running;
}

bool MainWindowDBusAdaptor::recording(const QString& src)
{
    auto pipeline = src.isEmpty()? wnd->activePipeline: wnd->findPipeline(src);
    return pipeline && pipeline->recording;
}

QString MainWindowDBusAdaptor::src()
{
    return wnd->activePipeline? wnd->activePipeline->alias: QString();
}

void MainWindowDBusAdaptor::setSrc(const QString& value)
{
    auto pipeline = wnd->findPipeline(value);
    if (pipeline && pipeline != wnd->activePipeline)
    {
        wnd->onSwapSources(pipeline->displayWidget, nullptr);
    }
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

bool MainWindowDBusAdaptor::stopStudy()
{
    if (busy())
    {
        return wnd->confirmStopStudy();
    }

    return false;
}

bool MainWindowDBusAdaptor::takeSnapshot
    ( const QString &src
    , const QString &imageFileTemplate
    )
{
    return wnd->takeSnapshot(wnd->findPipeline(src), imageFileTemplate);
}

bool MainWindowDBusAdaptor::startRecord
    ( int duration
    , const QString &src
    , const QString &clipFileTemplate
    )
{
    return wnd->startRecord(duration, wnd->findPipeline(src), clipFileTemplate);
}

bool MainWindowDBusAdaptor::stopRecord(const QString &src)
{
    auto pipeline = src.isEmpty()? wnd->activePipeline: wnd->findPipeline(src);

    if (pipeline && pipeline->recording)
    {
        wnd->stopRecord(pipeline);
        return true;
    }

    return false;
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

