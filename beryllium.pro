# Copyright (C) 2013 Irkutsk Diagnostic Center.
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

isEmpty(PREFIX):  PREFIX   = /usr/local
DEFINES   += PREFIX=$$PREFIX

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
INCLUDEPATH += libqxt

DISTRO = $$system(cat /proc/version)

# GCC tuning
*-g++*:QMAKE_CXXFLAGS += -std=c++0x -Wno-multichar

win32 {
    INCLUDEPATH += c:/usr/include
    LIBS += c:/usr/lib/*.lib advapi32.lib netapi32.lib wsock32.lib
}

unix {
    CONFIG    += link_pkgconfig
    greaterThan(QT_MAJOR_VERSION, 4) {
        PKGCONFIG += Qt5GLib-2.0 Qt5GStreamer-0.10 Qt5GStreamerUi-0.10
    }
    else {
        PKGCONFIG += QtGLib-2.0 QtGStreamer-0.10 QtGStreamerUi-0.10
    }
    PKGCONFIG += gstreamer-0.10 gstreamer-base-0.10 gstreamer-interfaces-0.10

    # libmediainfo.pc adds UNICODE, but dcmtk isn't compatible with wchar,
    # so we can't use pkgconfig for this library
    LIBS += -lmediainfo -lzen
}

TARGET   = beryllium
TEMPLATE = app

SOURCES += aboutdialog.cpp \
    archivewindow.cpp \
    beryllium.cpp \
    mainwindow.cpp \
    mandatoryfieldgroup.cpp \
    mandatoryfieldssettings.cpp \
    patientdialog.cpp \
    physicianssettings.cpp \
    settings.cpp \
    storagesettings.cpp \
    studiessettings.cpp \
    videosettings.cpp \
    libqxt/qxtlineedit.cpp \
    libqxt/qxtspanslider.cpp \
    videoeditor.cpp \
    videoencodingprogressdialog.cpp \
    fix_mpeg_sys_type_find.cpp \
    typedetect.cpp \
    hotkeyedit.cpp \
    hotkeysettings.cpp \
    mouseshortcut.cpp \
    thumbnaillist.cpp

HEADERS += aboutdialog.h \
    archivewindow.h \
    comboboxwithpopupsignal.h \
    mainwindow.h \
    mandatoryfieldgroup.h \
    mandatoryfieldssettings.h \
    patientdialog.h \
    physicianssettings.h \
    product.h \
    qwaitcursor.h \
    settings.h \
    storagesettings.h \
    studiessettings.h \
    videosettings.h \
    libqxt/qxtlineedit.h \
    libqxt/qxtspanslider.h \
    libqxt/qxtspanslider_p.h \
    videoeditor.h \
    defaults.h \
    videoencodingprogressdialog.h \
    typedetect.h \
    hotkeyedit.h \
    hotkeysettings.h \
    mouseshortcut.h \
    thumbnaillist.h

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

        INSTALLS += translations sound shortcut icon man
}

include (touch/touch.pri)
include (dicom/dicom.pri) # Must be very last line of this file
