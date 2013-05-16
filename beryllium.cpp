#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QSettings>
#include <QIcon>
#include <QGst/Init>
#include "mainwindow.h"

#include "videosettings.h"

int main(int argc, char *argv[])
{
    int errCode = 0;

    // Pass some arguments to gStreamer.
    // For example --gst-debug-level=5
    //
    QGst::init(&argc, &argv);

    // QT init
    //
    QApplication app(argc, argv);
    app.setOrganizationName("irk-dc");
    app.setApplicationName("beryllium");
    app.setApplicationVersion("1.0");
    app.setWindowIcon(QIcon(":/buttons/beryllium"));

    // Translations
    //
    QTranslator  translator;
    QString      locale = QSettings().value("locale").toString();
    if (locale.isEmpty())
    {
        locale = QLocale::system().name();
    }

    if (translator.load(app.applicationFilePath() + "_" + locale))
    {
        app.installTranslator(&translator);
    }

    // UI scope
    //
    {
//        MainWindow wnd;
//        wnd.showMaybeMaximized();
        VideoSettings wnd;
        wnd.show();
        errCode = app.exec();
    }

    QGst::cleanup();
    return errCode;
}
