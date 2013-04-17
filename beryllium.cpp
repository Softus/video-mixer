#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QSettings>
#include <QIcon>
#include <QGst/Init>
#include "mainwindow.h"

#include <dcmtk/config/cfunix.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofcond.h>    /* for class OFCondition */
#include "dcmtk/config/osconfig.h" /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/oflog/oflog.h"

#define OFFIS_CONSOLE_APPLICATION "storescu"
static char rcsid[] = "$dcmtk: $";

int main(int argc, char *argv[])
{
    char *ddd[] = {"--debug", "--debug", nullptr};
    OFConsoleApplication appa(OFFIS_CONSOLE_APPLICATION , "DICOM storage (C-STORE) SCU", rcsid);
    OFCommandLine cmd;
    OFLog::addOptions(cmd);
    appa.parseCommandLine(cmd, 2, ddd, OFCommandLine::PF_ExpandWildcards);

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
        MainWindow wnd;
        wnd.showMaybeMaximized();
        errCode = app.exec();
    }

    QGst::cleanup();
    return errCode;
}
