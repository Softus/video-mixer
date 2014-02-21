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

QT += core gui dbus
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets opengl

unix: DISTRO = $$system(cat /proc/version)

# GCC tuning
*-g++*:QMAKE_CXXFLAGS += -std=c++0x -Wno-multichar

win32 {
    INCLUDEPATH += c:/usr/include
    QMAKE_LIBDIR += c:/usr/lib
    LIBS += advapi32.lib netapi32.lib wsock32.lib
}
unix {
    LIBS += -lX11
}

CONFIG += link_pkgconfig
greaterThan(QT_MAJOR_VERSION, 4) {
    PKGCONFIG += Qt5GLib-2.0 Qt5GStreamer-0.10 Qt5GStreamerUi-0.10
}
else {
    PKGCONFIG += QtGLib-2.0 QtGStreamer-0.10 QtGStreamerUi-0.10
}
PKGCONFIG += gstreamer-0.10 gstreamer-base-0.10 gstreamer-interfaces-0.10 gstreamer-pbutils-0.10 gio-2.0

TARGET   = beryllium
TEMPLATE = app

INCLUDEPATH += libqxt
DEFINES += QXT_STATIC

SOURCES += aboutdialog.cpp \
    archivewindow.cpp \
    beryllium.cpp \
    confirmationsettings.cpp \
    fix_mpeg_sys_type_find.cpp \
    hotkeyedit.cpp \
    hotkeysettings.cpp \
    libqxt/qxtconfirmationmessage.cpp \
    libqxt/qxtlineedit.cpp \
    libqxt/qxtspanslider.cpp \
    mainwindow.cpp \
    mainwindowdbusadaptor.cpp \
    mandatoryfieldgroup.cpp \
    mandatoryfieldssettings.cpp \
    mouseshortcut.cpp \
    patientdatadialog.cpp \
    physicianssettings.cpp \
    settings.cpp \
    sound.cpp \
    storagesettings.cpp \
    studiessettings.cpp \
    thumbnaillist.cpp \
    typedetect.cpp \
    videoeditor.cpp \
    videoencodingprogressdialog.cpp \
    videorecordsettings.cpp \
    videosettings.cpp

HEADERS += aboutdialog.h \
    archivewindow.h \
    comboboxwithpopupsignal.h \
    confirmationsettings.h \
    defaults.h \
    hotkeyedit.h \
    hotkeysettings.h \
    libqxt/qxtconfirmationmessage.h \
    libqxt/qxtlineedit.h \
    libqxt/qxtspanslider.h \
    libqxt/qxtspanslider_p.h \
    mainwindowdbusadaptor.h \
    mainwindow.h \
    mandatoryfieldgroup.h \
    mandatoryfieldssettings.h \
    mouseshortcut.h \
    patientdatadialog.h \
    physicianssettings.h \
    product.h \
    qwaitcursor.h \
    settings.h \
    sound.h \
    storagesettings.h \
    studiessettings.h \
    thumbnaillist.h \
    typedetect.h \
    videoeditor.h \
    videoencodingprogressdialog.h \
    videorecordsettings.h \
    videosettings.h

FORMS   +=

RESOURCES    += beryllium.qrc
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

include (touch/touch.pri)
include (dicom/dicom.pri) # Must be very last line of this file
