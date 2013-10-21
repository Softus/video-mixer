#include "videoencodingprogressdialog.h"
#include <QMessageBox>
#include <QTimer>

#include <QGlib/Connect>
#include <QGlib/Error>
#include <QGst/Bus>
#include <QGst/Query>

#define SLIDER_SCALE 20000L

VideoEncodingProgressDialog::VideoEncodingProgressDialog
    ( QGst::PipelinePtr& pipeline
    , qint64 duration
    , QWidget* parent
    )
    : QProgressDialog(parent)
    , pipeline(pipeline)
    , duration(duration)
    , positionTimer(nullptr)
{
    QGlib::connect(pipeline->bus(), "message", this, &VideoEncodingProgressDialog::onBusMessage);
    pipeline->bus()->addSignalWatch();
    positionTimer = new QTimer(this);
    connect(positionTimer, SIGNAL(timeout()), this, SLOT(onPositionChanged()));
    positionTimer->start(200);
}

void VideoEncodingProgressDialog::onPositionChanged()
{
    QGst::PositionQueryPtr queryPos = QGst::PositionQuery::create(QGst::FormatTime);
    if (pipeline->query(queryPos))
    {
        setValue(queryPos->position() * SLIDER_SCALE / duration);
    }
}

void VideoEncodingProgressDialog::onBusMessage(const QGst::MessagePtr& message)
{
    switch (message->type())
    {
    case QGst::MessageStateChanged:
        onStateChange(message.staticCast<QGst::StateChangedMessage>());
        break;
    case QGst::MessageEos: //End of stream. We reached the end of the file.
        accept();
        break;
    case QGst::MessageError:
        {
            auto ex = message.staticCast<QGst::ErrorMessage>()->error();
            auto obj = message->source();
            QString msg;
            if (obj)
            {
                msg.append(obj->property("name").toString()).append(' ');
            }
            msg.append(ex.message());

            qCritical() << msg;
#ifndef Q_WS_WIN
            // Showing a message box under Microsoft (R) Windows (TM) breaks everything,
            // since it becomes the active one and the video output goes here.
            // So we can no more then hide this error from the user.
            //
            QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
#endif
            cancel();
        }
    default:
        qDebug() << message->typeName() << " " << message->source()->property("name").toString();
        break;
    }
}

void VideoEncodingProgressDialog::onStateChange(const QGst::StateChangedMessagePtr& message)
{
    qDebug() << message->typeName() << " " << message->source()->property("name").toString() << " " << message->oldState() << " => " << message->newState();
}
