# Copyright (C) 2015-2016 Softus Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; version 2.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

isEmpty(PREFIX): PREFIX   = /usr/local

# GCC tuning
*-g++*:QMAKE_CXXFLAGS += -std=c++0x -Wno-multichar

CONFIG += link_pkgconfig

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
    PKGCONFIG += Qt5GLib-2.0 Qt5GStreamer-1.0 Qt5GStreamerUi-1.0 gstreamer-1.0
}
else {
    PKGCONFIG += QtGLib-2.0 QtGStreamer-0.10 QtGStreamerUi-0.10 gstreamer-0.10
}

TARGET    = video-mixer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += src/main.cpp \
    src/mixer.cpp

HEADERS += \
    src/mixer.h

unix {
    INSTALLS += target
    target.path = $$PREFIX/bin

    init.files = video-mixer.conf
    init.path = /etc/init

    man.files = video-mixer.1
    man.path = $$PREFIX/share/man/man1
    INSTALLS += man init
}
