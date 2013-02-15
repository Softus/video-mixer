#ifndef VLC_ON_QT_H
#define VLC_ON_QT_H

#include <vlc/vlc.h>

#include <QX11EmbedContainer>
#include <QWidget>

class QVBoxLayout;
class QPushButton;
class QTimer;
class QFrame;
class QSlider;

#define POSITION_RESOLUTION 10000

class Player : public QWidget
{
    Q_OBJECT
#ifdef Q_WS_X11
    QX11EmbedContainer *_videoWidget;
#else
    QFrame *_videoWidget;
#endif

    libvlc_instance_t *_vlcinstance;
    libvlc_media_player_t *_mp;
    libvlc_media_t *_m;

public:
    Player();
    ~Player();

public slots:
    void playFile(QString file);

};
#endif
