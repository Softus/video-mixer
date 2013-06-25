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
    greaterThan(QT_MAJOR_VERSION, 4) {
        PKGCONFIG += Qt5GLib-2.0 Qt5GStreamer-0.10 Qt5GStreamerUi-0.10
    }
    else {
        PKGCONFIG += QtGLib-2.0 QtGStreamer-0.10 QtGStreamerUi-0.10
    }
    PKGCONFIG += gstreamer-0.10 gstreamer-base-0.10

    # libmediainfo.pc adds UNICODE, but dcmtk isn't compatible with wchar,
    # so we can't use pkgconfig for this library
    LIBS += -lmediainfo -lzen
}

dicom {
    QT        += network
    unix:LIBS += -ldcmnet -lwrap -ldcmdata -loflog -lofstd -lssl -lz
    DEFINES   += WITH_DICOM

    SOURCES   += dcmclient.cpp \
    dcmconverter.cpp \
    detailsdialog.cpp \
    dicomdevicesettings.cpp \
    dicommppsmwlsettings.cpp \
    dicomserverdetails.cpp \
    dicomserversettings.cpp \
    dicomstoragesettings.cpp \
    transcyrillic.cpp \
    worklistcolumnsettings.cpp \
    worklist.cpp \
    worklistquerysettings.cpp

    HEADERS += comboboxwithpopupsignal.h \
    dcmclient.h \
    dcmconverter.h \
    detailsdialog.h \
    dicomdevicesettings.h \
    dicommppsmwlsettings.h \
    dicomserverdetails.h \
    dicomserversettings.h \
    dicomstoragesettings.h \
    transcyrillic.h \
    worklistcolumnsettings.h \
    worklist.h \
    worklistquerysettings.h
}

TARGET   = beryllium
TEMPLATE = app

SOURCES += aboutdialog.cpp \
    archivewindow.cpp \
    beryllium.cpp \
    mainwindow.cpp \
    patientdialog.cpp \
    physicianssettings.cpp \
    settings.cpp \
    storagesettings.cpp \
    studiessettings.cpp \
    videosettings.cpp

HEADERS += aboutdialog.h \
    archivewindow.h \
    mainwindow.h \
    patientdialog.h \
    physicianssettings.h \
    product.h \
    qwaitcursor.h \
    settings.h \
    storagesettings.h \
    studiessettings.h \
    videosettings.h

FORMS   +=

RESOURCES    += beryllium.qrc
TRANSLATIONS += beryllium_ru.ts
