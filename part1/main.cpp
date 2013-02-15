#include <QtGui/QApplication>

#include "vlc_on_qt.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Player p;
    p.resize(1280,1024);
    p.playFile("v4l2:///dev/video1");
    p.show();
    return a.exec();
}
