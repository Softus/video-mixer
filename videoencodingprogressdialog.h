#ifndef VIDEOENCODINGPROGRESSDIALOG_H
#define VIDEOENCODINGPROGRESSDIALOG_H

#include <QProgressDialog>

#include <QGst/Message>
#include <QGst/Pipeline>

class VideoEncodingProgressDialog : public QProgressDialog
{
    Q_OBJECT
public:
    VideoEncodingProgressDialog
        ( QGst::PipelinePtr& pipeline
        , qint64 duration
        , QWidget* parent = nullptr
        );

signals:

public slots:
    void onPositionChanged();

private:
    void onBusMessage(const QGst::MessagePtr& message);
    void onStateChange(const QGst::StateChangedMessagePtr& message);

private:
    QGst::PipelinePtr& pipeline;
    qint64             duration;
    QTimer*            positionTimer;
};

#endif // VIDEOENCODINGPROGRESSDIALOG_H
