#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

#include <QGst/Init>

#include "mainwindow.h"
#include "product.h"

int main(int argc, char *argv[])
{
    int errCode = 0;

    // Pass some arguments to gStreamer.
    // For example --gst-debug-level=5
    //
    QGst::init(&argc, &argv);

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
//    app.setStyleSheet(settings.value("css", "*[mandatoryField=\"true\"] { background-color: pink }").toString());

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
            translator.load("/usr/share/" PRODUCT_SHORT_NAME "/translations/" PRODUCT_SHORT_NAME "_" + locale))
    {
        app.installTranslator(&translator);
    }

    // UI scope
    //
    {
        MainWindow wnd;
        wnd.show();
        errCode = app.exec();
    }

    QGst::cleanup();
    return errCode;
}
