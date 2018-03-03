Video mixer
===========

[![buddy pipeline](https://app.buddy.works/pbludov/video-mixer/pipelines/pipeline/127820/badge.svg?token=bf26fe8fed990190f11227bb2aa0c7d1e71118737795eed7b5069fff7106a015)](https://app.buddy.works/pbludov/video-mixer/pipelines/pipeline/127820)
[![Build Status](https://api.travis-ci.org/Softus/video-mixer.svg?branch=master)](https://travis-ci.org/Softus/video-mixer)
[![Build status](https://ci.appveyor.com/api/projects/status/0ew19agatl87brwj?svg=true)](https://ci.appveyor.com/project/pbludov/video-mixer)

Introduction
============

Video mixer is a small daemon written in c++.
It combines multiple video streams into one. For example, four input
streams stm1, stm2, stm3, stm4 may be combined into one who looks like:
```
+----------------+
| +----+  +----+ |
| |stm1|  |stm2| |
| +----+  +----+ |
|                |
| +----+  +----+ |
| |stm3|  |stm4| |
| +----+  +----+ |
+----------------+
```
May be used with (but not limited to) [gst-streaming-server](https://gstreamer.freedesktop.org/modules/gst-streaming-server.html).

Requirements
============

* [Qt](http://qt-project.org/) 5.0.2 or higher;
* [GStreamer](http://gstreamer.freedesktop.org/) 1.6 or higher;
* [QtGstreamer](http://gstreamer.freedesktop.org/modules/qt-gstreamer.html) 1.2 or higher;

Installation
============

Debian/Ubuntu/Mint
------------------

* Build dependecies

        sudo apt install -y lsb-release debhelper fakeroot libboost-dev libgstreamer1.0-dev \
          libgstreamer-plugins-base1.0-dev libqt5gstreamer-dev qttools5-dev-tools qt5-default

* Make video-mixer from the source

        qmake video-mixer.pro
        make
        sudo make install

* Create Package

        cp docs/* debian/
        dpkg-buildpackage -us -uc -tc -I.git -I*.yml -rfakeroot

SUSE/Open SUSE
--------------

* Build dependecies

        sudo install -y lsb-release rpm-build make libqt5-linguist libqt5-qtbase-devel gstreamer-plugins-qt5-devel

* Make video-mixer from the source

        qmake-qt5 video-mixer.pro
        make
        sudo make install

* Create Package

        tar czf ../video-mixer.tar.gz --exclude=debian --exclude=*.yml *
        rpmbuild -ta ../video-mixer.tar.gz

CentOS
------

* Build dependecies

        sudo yum install -y redhat-lsb rpm-build git make cmake gcc-c++ boost-devel \
          gstreamer1-plugins-base-devel qt5-qtdeclarative-devel gstreamer1-devel qt5-qtbase-devel qt5-linguist

* Build 3-rd party libraries

        # qt-gstreamer
        git clone https://anongit.freedesktop.org/git/gstreamer/qt-gstreamer.git
        cd qt-gstreamer
        mkdir build
        cd build
        cmake -Wno-dev .. -DCMAKE_INSTALL_PREFIX=/usr -DQT_VERSION=5
        sudo cmake --build . --target install
        cd ../..

* Make video-mixer from the source

        qmake-qt5 video-mixer.pro
        make
        sudo make install

* Create Package

        tar czf ../video-mixer.tar.gz --exclude=debian --exclude=qt-gstreamer --exclude=*.yml *
        rpmbuild -ta ../video-mixer.tar.gz

Fedora
------

* Build dependecies

        sudo dnf install -y redhat-lsb rpm-build make gcc-c++ gstreamer1-devel qt5-qtbase-devel qt5-gstreamer-devel qt5-linguist

* Make video-mixer from the source

        qmake-qt5 video-mixer.pro
        make
        sudo make install

* Create Package

        tar czf ../video-mixer.tar.gz --exclude=debian --exclude=*.yml *
        rpmbuild -ta ../video-mixer.tar.gz

Mageia
------

* Build dependecies

        sudo dnf install -y lsb-release rpm-build git make cmake gcc-c++ \
          qttools5 lib64qt5-gstreamer-devel lib64boost-devel \
          lib64gstreamer1.0-devel lib64gstreamer-plugins-base1.0-devel lib64qt5base5-devel

* Make video-mixer from the source

        qmake-qt5 video-mixer.pro
        make
        sudo make install

* Create Package

        tar czf ../video-mixer.tar.gz --exclude=cache* --exclude=debian --exclude=*.yml * && rpmbuild -ta ../video-mixer.tar.gz
        rpmbuild -ta ../video-mixer.tar.gz

Windows (Visual Studio)
-----------------------

* Build dependecies

  * [CMake](https://cmake.org/download/)
  * [WiX Toolset](http://wixtoolset.org/releases/)
  * [pkg-config](http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/)
  * [GStreamer, GStreamer-sdk](https://gstreamer.freedesktop.org/data/pkg/windows/)
  * [Boost](https://sourceforge.net/projects/boost/files/boost/)
  * [Qt MSVC](https://download.qt.io/archive/qt/)
  * [QtGStreamer](https://anongit.freedesktop.org/git/gstreamer/qt-gstreamer.git)

* Build 3-rd party libraries

        # QtGStreamer
        set BOOST_DIR=<the path to boost headers>
        mkdir build && cd build
        cmake -Wno-dev .. -DCMAKE_INSTALL_PREFIX=c:\usr -DQT_VERSION=5 -DBoost_INCLUDE_DIR=%BOOST_DIR% -G "Visual Studio <version>"
        cmake --build . --target install

* Make video-mixer from the source

        lrelease-qt5 *.ts
        qmake-qt5 INCLUDEDIR+=%BOOST_DIR%
        nmake -f Makefile.Release

* Create Package

        copy \usr\bin\*.dll release\
        copy <the path to qt folder>\bin\*.dll Release\
        xcopy /s <the path to qt folder>\plugins Release\

        set QT_RUNTIME=QtRuntime32.wxi
        wix\build.cmd

Note that both the GStreamer & Qt must be built with exactly the same
version of the MSVC. For example, if GStreamer is build with MSVC 2010,
the Qt version should must be any from 5.0 till 5.5.

Windows (MinGW)
---------------

* Build dependecies

  * [CMake](https://cmake.org/download/)
  * [WiX Toolset](http://wixtoolset.org/releases/)
  * [GStreamer, GStreamer-sdk](https://gstreamer.freedesktop.org/data/pkg/windows/)
  * [Boost](https://sourceforge.net/projects/boost/files/boost/)
  * [Qt 5.0.2 MinGW](https://download.qt.io/archive/qt/5.0/5.0.2/)
  * [QtGStreamer](https://anongit.freedesktop.org/git/gstreamer/qt-gstreamer.git)

* Build 3-rd party libraries

        # QtGStreamer
        set BOOST_DIR=<the path to boost headers>
        cmake -Wno-dev .. -DCMAKE_INSTALL_PREFIX=c:\usr -DQT_VERSION=5 -DBoost_INCLUDE_DIR=%BOOST_DIR% -G "MinGW Makefiles"
        cmake --build . --target install

* Make video-mixer from the source

        lrelease-qt5 *.ts
        qmake-qt5 INCLUDEDIR+=%BOOST_DIR%
        min32gw-make -f Makefile.Release

* Create Package

        copy \usr\bin\*.dll release\
        copy C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\*.dll Release\
        xcopy /s C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins Release\

        set QT_RUNTIME=qt-5.0.2_mingw-4.7.wxi
        wix\build.cmd

Note that both the GStreamer & Qt must be built with exactly the same
version of the GCC. For example, if GStreamer is build with GCC 4.7.3,
the Qt version should must be 5.0.
