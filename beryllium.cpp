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

#ifdef WITH_TOUCH
#include "touch/clickfilter.h"
#endif

#include "product.h"
#include "archivewindow.h"
#include "mainwindow.h"
#include "mainwindowdbusadaptor.h"
#include "videoeditor.h"
#include "settings.h"

#include <signal.h>

#include <QApplication>
#include <QDBusConnection>
#include <QIcon>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#ifdef Q_WS_X11
#include <QX11Info>
#include <X11/Xlib.h>
#endif
#include <QGst/Init>

#ifdef WITH_DICOM
#define HAVE_CONFIG_H
#include <dcmtk/config/osconfig.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofconapp.h>
#include <dcmtk/oflog/oflog.h>
#endif

#ifdef Q_OS_WIN
#define DATA_FOLDER qApp->applicationDirPath()
#else
#define DATA_FOLDER qApp->applicationDirPath() + "/../share/" PRODUCT_SHORT_NAME
#endif

// The mpegps type finder is a bit broken,
// this code fixes it.
//
extern void fix_mpeg_sys_type_find();

volatile sig_atomic_t fatal_error_in_progress = 0;
void sighandler(int signum)
{
    // Since this handler is established for more than one kind of signal,
    // it might still get invoked recursively by delivery of some other kind
    // of signal.  Use a static variable to keep track of that.
    //
    if (fatal_error_in_progress)
    {
        raise(signum);
    }

    fatal_error_in_progress = 1;
    QSettings().setValue("safe-mode", true);

    // Now call default signal handler which generates the core file.
    //
    signal(signum, SIG_DFL);
}

int main(int argc, char *argv[])
{
    int errCode = 0;

    signal(SIGSEGV, sighandler);
    // Pass some arguments to gStreamer.
    // For example --gst-debug-level=5
    //
    QGst::init(&argc, &argv);
    fix_mpeg_sys_type_find();

#ifdef WITH_DICOM
    // Pass some arguments to dcmtk.
    // For example --log-level trace
    // or --log-config log.cfg
    // See http://support.dcmtk.org/docs-dcmrt/file_filelog.html for details
    //
    OFConsoleApplication dcmtkApp(PRODUCT_SHORT_NAME);
    OFCommandLine cmd;
    OFLog::addOptions(cmd);
    cmd.addOption("--sync",           nullptr,    "Run the program in X synchronous mode."); // http://qt-project.org/doc/qt-4.8/debug.html
    cmd.addOption("--archive",        nullptr, 1, "folder name", "Start in archive mode");
    cmd.addOption("--edit-video",     nullptr, 1, "file name", "Start in video editing mode");
    cmd.addOption("--safe-mode",      nullptr,    "Run the program in safe mode");
    cmd.addOption("--settings",       nullptr,    "Run the program in edit settings mode");
    cmd.addOption("--auto-start",        "-a",    "Show the start study dialog");

    cmd.addOption("--study-id",         "-si", 1, "string",   "Study id");
    cmd.addOption("--patient-birthdate","-pb", 1, "yyyyMMdd", "Patient birth date");
    cmd.addOption("--patient-id",       "-pi", 1, "string",   "Patient Id");
    cmd.addOption("--patient-name",     "-pn", 1, "string",   "Patient name");
    cmd.addOption("--patient-sex",      "-ps", 1, "F|M",      "Patient sex");
    cmd.addOption("--physician",         "-p", 1, "string",   "Physician name");
    cmd.addOption("--study-desctiption","-sd", 1, "string",   "Study description");
    dcmtkApp.parseCommandLine(cmd, argc, argv);
    OFLog::configureFromCommandLine(cmd, dcmtkApp);
#endif

    // QT init
    //
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
    QApplication::setAttribute(Qt::AA_X11InitThreads);
    QApplication app(argc, argv);
    app.setOrganizationName(ORGANIZATION_SHORT_NAME);
    app.setApplicationName(PRODUCT_SHORT_NAME);
    app.setApplicationVersion(PRODUCT_VERSION_STR);
    app.setWindowIcon(QIcon(":/app/product"));

#ifdef Q_WS_X11
    if (app.arguments().indexOf("--sync") || qgetenv("DO_X_SYNCHRONIZE").toInt())
    {
         XSynchronize(QX11Info::display(), true);
    }
#endif

    // At this time it is safe to use QSettings
    //
    QSettings settings;
    bool fullScreen = settings.value("show-fullscreen").toBool();

    // Override some style sheets
    //
    app.setStyleSheet(settings.value("css").toString());

    // Translations
    //
    QTranslator  translator;
    QString      locale = settings.value("locale").toString();
    if (locale.isEmpty())
    {
        locale = QLocale::system().name();
    }

    // First, try to load localization in the current directory, if possible.
    // If failed, then try to load from app data path
    //
    if (translator.load(app.applicationDirPath() + "/" PRODUCT_SHORT_NAME "_" + locale) ||
        translator.load(DATA_FOLDER "/translations/" PRODUCT_SHORT_NAME "_" + locale))
    {
        app.installTranslator(&translator);
    }

    QWidget* wnd = nullptr;

    // UI scope
    //
    int idx = app.arguments().indexOf("--edit-video");
    if (idx >= 0 && ++idx < app.arguments().size())
    {
        wnd = new VideoEditor(app.arguments().at(idx));
    }
    else if (idx = app.arguments().indexOf("--settings"), idx >= 0)
    {
        wnd = new Settings;
    }
    else if (idx = app.arguments().indexOf("--archive"), idx >= 0)
    {
        auto wndArc = new ArchiveWindow;
        wndArc->updateRoot();
        wndArc->setPath(++idx < app.arguments().size()? app.arguments().at(idx): QDir::currentPath());
        wnd = wndArc;
    }
    else
    {
        auto bus = QDBusConnection::sessionBus();

        // Failed to register our service.
        // Another instance is running, or DBus is complitelly broken.
        //
        if (bus.registerService(PRODUCT_NAMESPACE) || !MainWindow::switchToRunningInstance())
        {
            auto wndMain = new MainWindow;

            // connect to DBus and register as an object
            //
            new MainWindowDBusAdaptor(wndMain);
            bus.registerObject("/com/irkdc/Beryllium/Main", wndMain);
            wnd = wndMain;
        }
    }

    if (wnd)
    {
#ifdef WITH_TOUCH
        ClickFilter filter;
        wnd->installEventFilter(&filter);
        wnd->grabGesture(Qt::TapAndHoldGesture);
#endif
        fullScreen? wnd->showFullScreen(): wnd->show();
        errCode = app.exec();
        delete wnd;
    }

    QGst::cleanup();
    return errCode;
}
