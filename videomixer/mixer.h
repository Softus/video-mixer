#ifndef MIXER_H
#define MIXER_H

#include <QObject>
#include <QRect>
#include <QGst/Buffer>
#include <QGst/Element>
#include <QGst/Message>
#include <QGst/Pad>
#include <QGst/Pipeline>

class Mixer : public QObject
{
    Q_OBJECT
    QGst::PipelinePtr   pl;
    QString             dstUri;
    QMap<QString, QPair<QString, bool>> srcMap;
    int                 updateTimerId;
    int                 width;
    int                 height;
    int                 delay;
    int                 padding;
    QRect               margins;
    QString             encoder;
    QString             message;

    void onBusMessage(const QGst::MessagePtr& msg);
    void onHttpFrame(const QGst::BufferPtr&, const QGst::PadPtr& padding);
    void buildPipeline();
    void releasePipeline();
    void restart(int delay);

public:
    Mixer(const QString& group, QObject *parent = 0);
    ~Mixer();

protected:
    virtual void timerEvent(QTimerEvent *);

signals:

public slots:
};

#endif // MIXER_H
