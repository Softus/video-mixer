#include "mixer.h"

#include <signal.h>
#include <QCoreApplication>
#include <QDebug>
#include <QSettings>
#include <QStringList>

#include <QGst/Init>
#include <QGlib/Error>

void sighandler(int code)
{
    qApp->exit(code);
}

int main(int argc, char *argv[])
{
    int errCode;

    // QGStreamer stuff
    //
    QGst::init(&argc, &argv);

    QCoreApplication::setOrganizationName("dc.baikal.ru");
    QCoreApplication::setApplicationName("videomixer");
    QCoreApplication app(argc, argv);

    QSettings settings;
    QList<Mixer*> mixers;

    Q_FOREACH (auto group, settings.childGroups())
    {
        qDebug() << group;

        try
        {
            mixers.push_back(new Mixer(group));
        }
        catch (const QGlib::Error& ex)
        {
            qCritical() << group << " error:" << ex.message();
        }
    }

    signal(SIGINT, sighandler);

    errCode = app.exec();
    Q_FOREACH (auto mixer, mixers)
    {
        delete mixer;
    }

    QGst::cleanup();
    return errCode;
}
