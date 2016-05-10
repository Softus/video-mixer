#include "mixer.h"

#include <signal.h>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStringList>
#include <QThread>

#include <QGst/Init>
#include <QGlib/Error>
#include <gst/gst.h>

#define MAX_GROUPS 32
#define ORGANIZATION_DOMAIN "dc.baikal.ru"
#define PRODUCT_SHORT_NAME "videomixer"

static gchar *groupName = nullptr;
static gchar *configDir = nullptr;

static GOptionEntry options[] = {
    {"group", 'g', 0, G_OPTION_ARG_STRING, (gpointer)&groupName,
        QT_TRANSLATE_NOOP_UTF8("cmdline", "Group to mix."), QT_TRANSLATE_NOOP_UTF8("cmdline", "STRING")},
    {"config", 'c', 0, G_OPTION_ARG_STRING, (gpointer)&configDir,
        QT_TRANSLATE_NOOP_UTF8("cmdline", "Path to settings root."), QT_TRANSLATE_NOOP_UTF8("cmdline", "DIR")},
    {nullptr, '\x0', 0, G_OPTION_ARG_NONE, nullptr,
        nullptr, nullptr},
};

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

QProcess* startMinion(const QString& app, const QString group)
{
    auto p = new QProcess();
    QStringList args;
    if (configDir)
    {
        args << "--config" << configDir;
    }
    args << "--group" << group;
    qDebug() << "[Gryu] Starting" << app << args;
    p->setProcessChannelMode(QProcess::ForwardedChannels);
    p->start(app, args);
    if (!p->waitForStarted())
    {
        delete p;
        p = nullptr;
    }
    return p;
}

static int gryuMode(const QString& app)
{
    // Gryu mode
    //
    QSettings settings;
    QMap<QString, QProcess*> minions;

    Q_FOREACH (auto group, settings.childGroups())
    {
        auto p = startMinion(app, group);
        if (!p)
        {
            qCritical() << "[Gryu] Failed to start" << group;
            continue;
        }
        minions[group] = p;
    }

    if (minions.isEmpty())
    {
        qCritical() << "[Gryu] No groups defined";
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
                qDebug() << "[Gryu]" << group << "has been finished";
                if (p->exitCode())
                {
                    qCritical() << "[Gryu] Will not restart minion" << group << "err" << p->exitCode() << p->errorString();
                    delete p;
                    minions.remove(group);
                    break;
                }

                delete p;
                p = startMinion(app, group);
                if (!p)
                {
                    qCritical() << "[Gryu] Failed to restart minion" << group;
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
        qDebug() << "[Gryu] minion" << minion.key() << p->pid() << "killed";
        p->kill();
        p->waitForFinished(100);
        delete p;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(PRODUCT_SHORT_NAME);

    // Pass some arguments to gstreamer.
    // For example --gst-debug-level=5
    //
    GError* err = nullptr;
    auto ctx = g_option_context_new("");
    g_option_context_add_main_entries(ctx, options, PRODUCT_SHORT_NAME);
    g_option_context_add_group(ctx, gst_init_get_option_group());
    g_option_context_set_ignore_unknown_options(ctx, false);
    g_option_context_parse(ctx, &argc, &argv, &err);
    g_option_context_free(ctx);

    if (err)
    {
        g_print(QT_TRANSLATE_NOOP_UTF8("cmdline", "Error initializing: %s\n"), GST_STR_NULL(err->message));
        g_error_free(err);
        return 1;
    }

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    if (configDir)
    {
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, configDir);
    }

    int exitCode;
    if (groupName)
    {
        // QGStreamer stuff
        //
        QGst::init();

        QCoreApplication app(argc, argv);
        try
        {
            Mixer mixer(groupName);
            exitCode = app.exec();
        }
        catch (const QGlib::Error& ex)
        {
            qCritical() << ex.message();
            exitCode = ex.code();
        }

        QGst::cleanup();
    }
    else
    {
        exitCode = gryuMode(argv[0]);
    }

    return exitCode;
}
