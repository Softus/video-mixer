#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QIcon>
#include <QGst/Init>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // QT init
    //
    QApplication app(argc, argv);
    app.setOrganizationName("irk-dc");
    app.setApplicationName("beryllium");
    app.setApplicationVersion("1.0");
    app.setWindowIcon(QIcon(":/buttons/beryllium"));

    // Pass some arguments to gStreamer.
    // For example --gst-debug-level=5
    //
    QGst::init(&argc, &argv);

    // Translations
    //
    QTranslator  translator;
//    qDebug() << app.applicationFilePath() + "_" + QLocale::system().name();
    if (translator.load(app.applicationFilePath() + "_" + QLocale::system().name()))
    {
        app.installTranslator(&translator);
    }

    // Start the UI
    //
    MainWindow wnd;
#ifdef QT_DEBUG
    wnd.showNormal();
#else
    wnd.showMaximized();
#endif

    int errCode = app.exec();
    QGst::cleanup();
    return errCode;
}
