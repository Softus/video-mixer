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
    QString             group;
    QMap<QString, QPair<QString, bool>> srcMap;
    int                 updateTimerId;
    int                 width;
    int                 height;
    int                 delay;
    int                 padding;
    bool                zOrderFix;
    QRect               margins;
    QString             source;
    QString             sink;
    QString             decoder;
    QString             encoder;
    QString             message;

    void onBusMessage(const QGst::MessagePtr& msg);
    void onHttpFrame(const QGst::BufferPtr&, const QGst::PadPtr& padding);
    void buildPipeline();
    void releasePipeline();
    QString buildBackground(bool inactive, int rowSize);

public:
    Mixer(const QString& group, QObject *parent = 0);
    ~Mixer();

protected:
    virtual void timerEvent(QTimerEvent *);

signals:
    void restart(int delay);

private slots:
    void onRestart(int delay);

};

#endif // MIXER_H
