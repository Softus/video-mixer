#ifndef SOUND_H
#define SOUND_H

#include <QObject>

#include <QGst/Pipeline>
#include <gst/gstdebugutils.h>

class Sound : public QObject
{
    Q_OBJECT
public:
    explicit Sound(QObject *parent = 0);
    ~Sound();

    bool play(const QString& filePath);

private:
    QGst::PipelinePtr pipeline;
};

#endif // SOUND_H
