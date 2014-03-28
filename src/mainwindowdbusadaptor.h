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

#ifndef MAINWINDOWDBUSADAPTOR_H
#define MAINWINDOWDBUSADAPTOR_H

#include <QDBusAbstractAdaptor>

class MainWindow;

class MainWindowDBusAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "ru.baikal.dc.beryllium.Main") // Preprocessor won't work here
    Q_PROPERTY(bool busy READ busy)
    Q_PROPERTY(bool recording READ recording)

private:
    MainWindow *wnd;

public:
    MainWindowDBusAdaptor(MainWindow *wnd);

public slots:
    bool busy();
    bool recording();
    bool startStudy
        ( const QString &accessionNumber = QString()
        , const QString &id = QString(), const QString &name = QString()
        , const QString &sex = QString(), const QString &birthday = QString()
        , const QString &physician = QString(), const QString &studyType = QString()
        , bool autoStart = true
        );
    bool stopStudy();
    bool takeSnapshot
        ( const QString &imageFileTemplate = QString()
        );
    bool startRecord
        ( int duration = 0
        , const QString &clipFileTemplate = QString()
        );
    bool stopRecord();
    QString value
        ( const QString &name
        );
    void setValue
        ( const QString &name
        , const QString &value = QString()
        );
    void setConfigPath
        ( const QString &path
        );
};

#endif // MAINWINDOWDBUSADAPTOR_H
