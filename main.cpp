#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTranslator  translator;

    if (translator.load(app.applicationFilePath() + "_" + QLocale::system().name())) {
        app.installTranslator(&translator);
    }

    MainWindow wnd;
    wnd.showMaximized();
    return app.exec();
}
