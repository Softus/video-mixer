# Copyright (C) 2013-2014 Irkutsk Diagnostic Center.
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
DEFINES += PREFIX=$$PREFIX

QT += core gui dbus opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# GCC tuning
*-g++*:QMAKE_CXXFLAGS += -std=c++0x -Wno-multichar

win32 {
    INCLUDEPATH += c:/usr/include
    QMAKE_LIBDIR += c:/usr/lib
    LIBS += advapi32.lib netapi32.lib wsock32.lib

    USERNAME    = $$(USERNAME)
    OS_DISTRO   = windows
    OS_REVISION = $$system(wmic os get version | gawk -F . \'NR==2\{print \$1 \".\" \$2\}\')
}
unix {
    LIBS += -lX11

    USERNAME    = $$(USER)
    OS_DISTRO   = $$system(lsb_release -is | awk \'\{print \$1\}\')
    OS_REVISION = $$system(lsb_release -rs)
}
DEFINES += OS_DISTRO=$$OS_DISTRO OS_REVISION=$$OS_REVISION USERNAME=$$USERNAME

CONFIG += link_pkgconfig
greaterThan(QT_MAJOR_VERSION, 4) {
    PKGCONFIG += Qt5GLib-2.0 Qt5GStreamer-0.10 Qt5GStreamerUi-0.10
}
else {
    PKGCONFIG += QtGLib-2.0 QtGStreamer-0.10 QtGStreamerUi-0.10
}
PKGCONFIG += gstreamer-0.10 gstreamer-base-0.10 gstreamer-interfaces-0.10 gstreamer-pbutils-0.10 gio-2.0 opencv

TARGET   = beryllium
TEMPLATE = app

INCLUDEPATH += libqxt
DEFINES += QXT_STATIC

SOURCES += \
    libqxt/qxtconfirmationmessage.cpp \
    libqxt/qxtlineedit.cpp \
    libqxt/qxtspanslider.cpp \
    gst/motioncells/motioncells_wrapper.cpp \
    gst/motioncells/MotionCells.cpp \
    gst/motioncells/gstmotioncells.cpp \
    gst/gst.cpp \
    gst/mpeg_sys_type_find.cpp \
    src/settings/debugsettings.cpp \
    src/settings/hotkeysettings.cpp \
    src/settings.cpp \
    src/settings/confirmationsettings.cpp \
    src/settings/mandatoryfieldssettings.cpp \
    src/settings/physicianssettings.cpp \
    src/settings/storagesettings.cpp \
    src/settings/studiessettings.cpp \
    src/settings/videorecordsettings.cpp \
    src/settings/videosourcesettings.cpp \
    src/aboutdialog.cpp \
    src/archivewindow.cpp \
    src/beryllium.cpp \
    src/hotkeyedit.cpp \
    src/mainwindow.cpp \
    src/mainwindowdbusadaptor.cpp \
    src/mandatoryfieldgroup.cpp \
    src/patientdatadialog.cpp \
    src/sound.cpp \
    src/thumbnaillist.cpp \
    src/typedetect.cpp \
    src/videoeditor.cpp \
    src/videoencodingprogressdialog.cpp \
    libqxt/qxtglobalshortcut.cpp \
    src/smartshortcut.cpp

macx: SOURCES += libqxt/mac/qxtglobalshortcut_mac.cpp
unix: SOURCES += libqxt/x11/qxtglobalshortcut_x11.cpp
win32:SOURCES += libqxt/win/qxtglobalshortcut_win.cpp

HEADERS += \
    libqxt/qxtconfirmationmessage.h \
    libqxt/qxtlineedit.h \
    libqxt/qxtspanslider.h \
    libqxt/qxtspanslider_p.h \
    gst/motioncells/motioncells_wrapper.h \
    gst/motioncells/MotionCells.h \
    gst/motioncells/gstmotioncells.h \
    src/settings/debugsettings.h \
    src/settings/hotkeysettings.h \
    src/settings.h \
    src/settings/videosourcesettings.h \
    src/settings/videorecordsettings.h \
    src/settings/studiessettings.h \
    src/settings/storagesettings.h \
    src/settings/physicianssettings.h \
    src/settings/mandatoryfieldssettings.h \
    src/settings/confirmationsettings.h \
    src/aboutdialog.h \
    src/archivewindow.h \
    src/comboboxwithpopupsignal.h \
    src/defaults.h \
    src/hotkeyedit.h \
    src/mainwindow.h \
    src/mainwindowdbusadaptor.h \
    src/mandatoryfieldgroup.h \
    src/patientdatadialog.h \
    src/product.h \
    src/qwaitcursor.h \
    src/sound.h \
    src/thumbnaillist.h \
    src/typedetect.h \
    src/videoeditor.h \
    src/videoencodingprogressdialog.h \
    libqxt/qxtglobalshortcut_p.h \
    libqxt/qxtglobalshortcut.h \
    libqxt/QxtGlobalShortcut \
    src/smartshortcut.h

FORMS   +=

RESOURCES    += \
    beryllium.qrc
TRANSLATIONS += beryllium_ru.ts

unix {
    INSTALLS += target
    target.path = $$PREFIX/bin

    shortcut.files = beryllium.desktop
    shortcut.path = $$PREFIX/share/applications
    icon.files = pixmaps/beryllium.png
    icon.path = $$PREFIX/share/icons
    man.files = beryllium.1
    man.path = $$PREFIX/share/man/man1
    translations.files = beryllium_ru.qm
    translations.path = $$PREFIX/share/beryllium/translations
    sound.files = sound/*
    sound.path = $$PREFIX/share/beryllium/sound
    dbus.files = com.irk-dc.beryllium.service
    dbus.path = $$PREFIX/share/dbus-1/services

    INSTALLS += dbus translations sound shortcut icon man
}

include (src/touch/touch.pri)
include (src/dicom/dicom.pri) # Must be very last line of this file
