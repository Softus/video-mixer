/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
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

#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#include <QGst/Init>

#ifdef WITH_DICOM
#define HAVE_CONFIG_H
#include <dcmtk/config/osconfig.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofconapp.h>
#include <dcmtk/oflog/oflog.h>
#endif

#ifdef WITH_TOUCH
#include "touch/clickfilter.h"
#endif

#include "mainwindow.h"
#include "videoeditor.h"
#include "product.h"

int main(int argc, char *argv[])
{
    int errCode = 0;

    // Pass some arguments to gStreamer.
    // For example --gst-debug-level=5
    //
    QGst::init(&argc, &argv);

#ifdef WITH_DICOM
    // Pass some arguments to dcmtk.
    // For example --log-level trace
    // or --log-config log.cfg
    // See http://support.dcmtk.org/docs-dcmrt/file_filelog.html for details
    //
    OFConsoleApplication dcmtkApp(PRODUCT_SHORT_NAME);
    OFCommandLine cmd;
    OFLog::addOptions(cmd);
    cmd.addOption("--edit-video","", 1);
    dcmtkApp.parseCommandLine(cmd, argc, argv);
    OFLog::configureFromCommandLine(cmd, dcmtkApp);
#endif

    // QT init
    //
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
    QApplication app(argc, argv);
    app.setOrganizationName(ORGANIZATION_SHORT_NAME);
    app.setApplicationName(PRODUCT_SHORT_NAME);
    app.setApplicationVersion(PRODUCT_VERSION_STR);
    app.setWindowIcon(QIcon(":/app/product"));

    // At this time it is safe to use QSettings
    //
    QSettings settings;

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
        translator.load(app.applicationDirPath() + "../share/" PRODUCT_SHORT_NAME "/translations/" PRODUCT_SHORT_NAME "_" + locale))
    {
        app.installTranslator(&translator);
    }

    // UI scope
    //
    int videoEditIdx = app.arguments().indexOf("--edit-video");
    if (videoEditIdx >= 0 && ++videoEditIdx < app.arguments().size())
    {
        VideoEditor wndEditor;
#ifdef WITH_TOUCH
        ClickFilter filter;
        wndEditor.installEventFilter(&filter);
        wndEditor.grabGesture(Qt::TapGesture);
#endif
        wndEditor.show();
        wndEditor.loadFile(app.arguments().at(videoEditIdx));
        errCode = app.exec();
    }
    else
    {
        MainWindow wnd;
#ifdef WITH_TOUCH
        ClickFilter filter;
        wnd.installEventFilter(&filter);
        wnd.grabGesture(Qt::TapGesture);
#endif
        wnd.show();
        errCode = app.exec();
    }

    QGst::cleanup();
    return errCode;
}
