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
    app.setApplicationName("cronica");
    app.setApplicationVersion("1.0");
    app.setWindowIcon(QIcon(":/buttons/cronica"));

    // GStreamer
    //
    QGst::init();

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
    wnd.show();

    int errCode = app.exec();
    QGst::cleanup();
    return errCode;
}
