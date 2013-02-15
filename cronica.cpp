/*
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
#ifdef QT_DEBUG
    wnd.showNormal();
#else
    wnd.showFullScreen();
#endif

    int errCode = app.exec();
    QGst::cleanup();
    return errCode;
}

*/


#include <iostream>
#include <QtCore/QCoreApplication>
#include <QGlib/Error>
#include <QGlib/Connect>
#include <QGst/Init>
#include <QGst/Bus>
#include <QGst/Pipeline>
#include <QGst/Parse>
#include <QGst/Message>
#include <QGst/Utils/ApplicationSink>
#include <QGst/Utils/ApplicationSource>


class MySink : public QGst::Utils::ApplicationSink
{
public:
    MySink(QGst::Utils::ApplicationSource *src)
        : QGst::Utils::ApplicationSink(), m_src(src) {}

protected:
    virtual void eos()
    {
        m_src->endOfStream();
    }

    virtual QGst::FlowReturn newBuffer()
    {
        m_src->pushBuffer(pullBuffer());
        return QGst::FlowOk;
    }

private:
    QGst::Utils::ApplicationSource *m_src;
};


class Player : public QCoreApplication
{
public:
    Player(int argc, char **argv);
    ~Player();

private:
    void onBusMessage(const QGst::MessagePtr & message);

private:
    QGst::Utils::ApplicationSource m_src;
    MySink m_sink;
    QGst::PipelinePtr pipeline1;
    QGst::PipelinePtr pipeline2;
};

Player::Player(int argc, char **argv)
    : QCoreApplication(argc, argv), m_sink(&m_src)
{
    QGst::init(&argc, &argv);

    const char *caps = "video/x-raw-yuv,format=YUY2,color-matrix=sdtv,chroma-site=mpeg2,width=320,height=240,framerate=30/1";

    /* source pipeline */
    QString pipe1Descr = QString("videotestsrc appsink name=\"mysink\" caps=\"%1\"").arg(caps);
    pipeline1 = QGst::Parse::launch(pipe1Descr).dynamicCast<QGst::Pipeline>();
    m_sink.setElement(pipeline1->getElementByName("mysink"));
    QGlib::connect(pipeline1->bus(), "message", this, &Player::onBusMessage);
    pipeline1->bus()->addSignalWatch();

    /* sink pipeline */
    QString pipe2Descr = QString("appsrc name=\"mysrc\" caps=\"%1\" ! ffmpegcolorspace ! autovideosink sync=false").arg(caps);
    pipeline2 = QGst::Parse::launch(pipe2Descr).dynamicCast<QGst::Pipeline>();
    m_src.setElement(pipeline2->getElementByName("mysrc"));
    QGlib::connect(pipeline2->bus(), "message", this, &Player::onBusMessage);
    pipeline2->bus()->addSignalWatch();

    /* start playing */
    pipeline1->setState(QGst::StatePlaying);
    pipeline2->setState(QGst::StatePlaying);
}

Player::~Player()
{
    pipeline1->setState(QGst::StateNull);
    pipeline2->setState(QGst::StateNull);
}

void Player::onBusMessage(const QGst::MessagePtr & message)
{
    qDebug() << message->typeName() << " " << message->source()->property("name").toString();

    switch (message->type())
    {
    case QGst::MessageStateChanged:
        {
            QGst::StateChangedMessagePtr m = message.staticCast<QGst::StateChangedMessage>();
            qDebug() << m->oldState() << " => " << m->newState();
		}
		break;
    case QGst::MessageEos:
        quit();
        break;
    case QGst::MessageError:
		qCritical() << message.staticCast<QGst::ErrorMessage>()->error() << " " << message.staticCast<QGst::ErrorMessage>()->debugMessage();
        break;
    default:
        qDebug() << message->type();
        break;
    }
}


int main(int argc, char **argv)
{
    Player p(argc, argv);
    return p.exec();
}
