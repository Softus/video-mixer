#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QSettings>
#include <QIcon>
#include <QGst/Init>
#include "mainwindow.h"
#ifdef WITH_DICOM
#include "worklist.h"
#endif

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
#ifdef WITH_DICOM
        Worklist wnd;
#else
        MainWindow wnd;
#endif

#ifdef QT_DEBUG
        wnd.showNormal();
#else
        wnd.showMaximized();
#endif
        errCode = app.exec();
    }

    QGst::cleanup();
    return errCode;
}
