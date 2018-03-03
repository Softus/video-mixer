# Copyright (C) 2015-2018 Softus Inc.
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

lessThan(QT_MAJOR_VERSION, 5): error (QT 5.0 or newer is required)

isEmpty(PREFIX): PREFIX   = /usr/local

QT        -= gui
CONFIG    += console link_pkgconfig c++11
CONFIG    -= app_bundle
PKGCONFIG += Qt5GLib-2.0 Qt5GStreamer-1.0 Qt5GStreamerUi-1.0 gstreamer-1.0
TARGET     = video-mixer
TEMPLATE   = app
OS_DISTRO  = $$lower($$system($$quote('lsb_release -is | cut -d" " -f1')))

SOURCES += src/main.cpp \
    src/mixer.cpp

HEADERS += \
    src/mixer.h

target.path = $$PREFIX/bin
man.files = video-mixer.1
man.path = $$PREFIX/share/man/man1
cfg.files=video-mixer.conf
cfg.path=/etc/xdg/softus.org
systemd.files=video-mixer.service
equals(OS_DISTRO, debian) | equals(OS_DISTRO, ubuntu) {
    systemd.path=/lib/systemd/system
} else {
    systemd.path=/usr/lib/systemd/system
}

INSTALLS += target man cfg systemd
