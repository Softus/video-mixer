#include "mixer.h"

#include <signal.h>
#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QSettings>
#include <QStringList>
#include <QThread>

#include <QGst/Init>
#include <QGlib/Error>

#define MAX_GROUPS 32

volatile sig_atomic_t runnung = 1;
static void sighandler(int signum)
{
    // Since this handler is established for more than one kind of signal,
    // it might still get invoked recursively by delivery of some other kind
    // of signal.  Use a static variable to keep track of that.
    //
    if (!runnung)
    {
        raise(signum);
    }

    runnung = 0;
    if (qApp) qApp->quit();

    // Now call default signal handler which generates the core file.
    //
    signal(signum, SIG_DFL);
}

static int minionMode(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    try
    {
        Mixer mixer(argv[1]);
        return app.exec();
    }
    catch (const QGlib::Error& ex)
    {
        qCritical() << ex.message();
        return ex.code();
    }
}

static int gryuMode(int /*argc*/, char *argv[])
{
    // Gryu mode
    //
    QSettings settings;
    QMap<QString, QProcess*> minions;

    Q_FOREACH (auto group, settings.childGroups())
    {
        qDebug() << "Starting" << group;
        auto p = new QProcess();
        p->start(argv[0], QStringList(group));
        if (!p->waitForStarted())
        {
            qCritical() << "Failed to start minion" << group << "err" << p->errorString();
            delete p;
            continue;
        }
        minions[group] = p;
    }

    if (minions.isEmpty())
    {
        qCritical() << "No groups defined";
        return 0;
    }

    while (runnung && !minions.isEmpty())
    {
        for (auto minion = minions.begin(); minion != minions.end(); ++minion)
        {
            auto p = minion.value();
            if (p->waitForFinished(100))
            {
                auto group = minion.key();
                qDebug() << "Restarting" << group;
                if (p->exitCode())
                {
                    qCritical() << "Failed to restart minion" << group << "err" << p->exitCode() << p->errorString();
                    delete p;
                    minions.remove(group);
                    break;
                }

                delete p;
                p = new QProcess();
                p->start(argv[0], QStringList(group));
                if (!p->waitForStarted())
                {
                    qCritical() << "Failed to restart minion" << group << "err" << p->errorString();
                    delete p;
                    minions.remove(group);
                }
                else
                {
                    minions[group] = p;
                }

                // Exit the loop since we may change the map internals and it is not safe
                // to continue the enumeration now.
                //
                break;
            }
        }
    }

    for (auto minion = minions.begin(); minion != minions.end(); ++minion)
    {
        auto p = minion.value();
        qDebug() << "minion" << minion.key() << p->pid() << "killed";
        p->kill();
        p->waitForFinished(100);
        delete p;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("dc.baikal.ru");
    QCoreApplication::setApplicationName("videomixer");

    // QGStreamer stuff
    //
    QGst::init(&argc, &argv);

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    int exitCode = argc > 1? minionMode(argc, argv): gryuMode(argc, argv);

    QGst::cleanup();
    return exitCode;
}
