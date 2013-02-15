#include "vlc_on_qt.h"

#include <QX11EmbedContainer>
#include <QVBoxLayout>
#include <QFrame>

Player::Player()
: QWidget()
{
    //preparation of the vlc command
    const char * const vlc_args[] = {
              "-I", "dummy", /* Don't use any interface */
              "--ignore-config", /* Don't use VLC's config */
              "--extraintf=logger", //log anything
              "--verbose=2", //be much more verbose then normal for debugging purpose
              "--sout=#duplicate{dst=display,dst={transcode{vcodec=MJPG,vb=10000,scale=1,acodec=none}:standard{access=file,mux=ts,dst='out.mpg'}}}",
//              "--sout=#duplicate{dst=display,dst={transcode{vcodec=h264,vb=5000,scale=1,acodec=none}:standard{access=file,mux=MP4,dst='out.mp4'}}}",
//              "--sout-ffmpeg-rc-buffer-size=16000000"
    };

#ifdef Q_WS_X11
    _videoWidget=new QX11EmbedContainer(this);
#else
    _videoWidget=new QFrame(this);
#endif

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(_videoWidget);
    setLayout(layout);

    //create a new libvlc instance
    _vlcinstance=libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);

    // Create a media player playing environement
    _mp = libvlc_media_player_new (_vlcinstance);
}

//desctructor
Player::~Player()
{
    /* Stop playing */
    libvlc_media_player_stop (_mp);

    /* Free the media_player */
    libvlc_media_player_release (_mp);

    libvlc_release (_vlcinstance);
}

void Player::playFile(QString file)
{
    /* Create a new LibVLC media descriptor */
    _m = libvlc_media_new_path(_vlcinstance, file.toUtf8());

    libvlc_media_player_set_media (_mp, _m);

    libvlc_media_player_set_xwindow (_mp, _videoWidget->winId());

    /* Play */
    libvlc_media_player_play (_mp);
}
