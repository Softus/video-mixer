#-------------------------------------------------
#
# Project beryllium
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# GCC tuning
*-g++*:QMAKE_CXXFLAGS += -std=c++0x -Wno-multichar

win32 {
    INCLUDEPATH += c:/usr/include
    LIBS += c:/usr/lib/*.lib advapi32.lib netapi32.lib wsock32.lib
}

unix {
    CONFIG    += link_pkgconfig
    PKGCONFIG += Qt5GLib-2.0 Qt5GStreamer-0.10 Qt5GStreamerUi-0.10 gstreamer-0.10 gstreamer-base-0.10
    LIBS      += -lmediainfo -lzen
}

dicom {
    QT        += network
    unix:LIBS += -ldcmnet -lwrap -ldcmdata -loflog -lofstd -lssl -lz
    DEFINES   += WITH_DICOM
    SOURCES   += worklist.cpp dcmclient.cpp detailsdialog.cpp \
    dicomdevicesettings.cpp \
    dicomserversettings.cpp \
    dicomstoragesettings.cpp \
    dicomserverdetails.cpp \
    worklistcolumnsettings.cpp \
    worklistquerysettings.cpp \
    dcmconverter.cpp

    HEADERS += worklist.h dcmclient.h detailsdialog.h \
    dicomdevicesettings.h \
    dicomserversettings.h \
    dicomstoragesettings.h \
    dicomserverdetails.h \
    worklistcolumnsettings.h \
    worklistquerysettings.h \
    dcmconverter.h
}

TARGET   = beryllium
TEMPLATE = app

SOURCES += beryllium.cpp mainwindow.cpp \
    videosettings.cpp \
    settings.cpp \
    storagesettings.cpp \
    studiessettings.cpp \
    patientdialog.cpp \
    aboutdialog.cpp \
    archivewindow.cpp \
    dicommppsmwlsettings.cpp \
    physicianssettings.cpp \
    transcyrillic.cpp
HEADERS += mainwindow.h qwaitcursor.h \
    videosettings.h \
    settings.h \
    storagesettings.h \
    studiessettings.h \
    patientdialog.h \
    aboutdialog.h \
    archivewindow.h \
    product.h \
    comboboxwithpopupsignal.h \
    dicommppsmwlsettings.h \
    physicianssettings.h \
    transcyrillic.h

FORMS   +=

RESOURCES    += beryllium.qrc
TRANSLATIONS += beryllium_ru.ts
