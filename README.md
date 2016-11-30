Video mixer
===========

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

1. Install build dependecies
    For QT5:
        sudo apt-get install libqt5opengl5-dev libqt5gstreamer1.0-dev \
        dpkg-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
    For QT4:
        sudo apt-get install libqt4-opengl-dev libqtgstreamer-dev \
        dpkg-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev

2. Make
  qmake video-mixer.pro
  make

3. Install
  sudo make install

SUSE/Open SUSE
--------------

1. Install build dependecies

        sudo zypper install make rpm-build qt-devel qt-gstreamer-devel \
        libQtGlib-devel dcmtk-devel tcp_wrappers-devel openssl-devel \
        mediainfo-devel

2. Make
  qmake-qt4 vide-omixer.pro
  make

3. Install
  sudo make install


Windows (Visual Studio)
-----------------------

1. Install build dependecies

  * [CMake](https://cmake.org/download/)
  * [WiX Toolset](http://wixtoolset.org/releases/)
  * [pkg-config](http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/)
  * [GStreamer, GStreamer-sdk](https://gstreamer.freedesktop.org/data/pkg/windows/)
  * [Boost](https://sourceforge.net/projects/boost/files/boost/)
  * [Qt 5.5 MSVC](https://download.qt.io/archive/qt/5.5/)
  * [QtGStreamer](https://github.com/detrout/qt-gstreamer.git)

2. Build 3-rd party libraries

        # QtGStreamer
        set BOOST_DIR=<the path to boost headers>
        mkdir build && cd build
        cmake -Wno-dev .. -DCMAKE_INSTALL_PREFIX=c:\usr -DQT_VERSION=5  -DBoost_INCLUDE_DIR=%BOOST_DIR% -G "Visual Studio 2010"
        cmake --build . --target install

3. Make

        qmake-qt5 INCLUDEDIR+=%BOOST_DIR%
        nmake -f Makefile.Release

4. Create Package

        copy \usr\bin\*.dll release\
        copy C:\Qt\Qt5.5.1\5.5.1\msvc2010\bin\*.dll Release\
        xcopy /s C:\Qt5.5.1\5.5.1\msvc2010\plugins Release\

        set QT_RUNTIME=QtRuntime32.wxi
        wix\build.cmd

Note that both the GStreamer & Qt must be built with exactly the same
version of the MSVC. For example, if GStreamer is build with MSVC 2010,
the Qt version should must be any from 5.0 till 5.5.

Windows (MinGW)
---------------

1. Install build dependecies

  * [CMake](https://cmake.org/download/)
  * [WiX Toolset](http://wixtoolset.org/releases/)
  * [GStreamer, GStreamer-sdk](https://gstreamer.freedesktop.org/data/pkg/windows/)
  * [Boost](https://sourceforge.net/projects/boost/files/boost/)
  * [Qt 5.0.2 MinGW](https://download.qt.io/archive/qt/5.0/5.0.2/)
  * [QtGStreamer (optional)](https://github.com/detrout/qt-gstreamer.git)

2. Build 3-rd party libraries

        # QtGStreamer
        set BOOST_DIR=<the path to boost headers>
        cmake -Wno-dev .. -DCMAKE_INSTALL_PREFIX=c:\usr -DQT_VERSION=5  -DBoost_INCLUDE_DIR=%BOOST_DIR% -G "MinGW Makefiles"
        cmake --build . --target install

3. Make

        qmake-qt5 INCLUDEDIR+=%BOOST_DIR%
        min32gw-make

4. Create Package

        copy \usr\bin\*.dll release\
        copy C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\*.dll Release\
        xcopy /s C:\Qt\Qt5.0.2\5.0.2\mingw47_32\plugins Release\

        set QT_RUNTIME=qt-5.0.2_mingw-4.7.wxi
        wix\build.cmd


Note that both the GStreamer & Qt must be built with exactly the same
version of the GCC. For example, if GStreamer is build with GCC 4.7.3,
the Qt version should must be 5.0.
