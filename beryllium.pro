#-------------------------------------------------
#
# Project beryllium
#
#-------------------------------------------------

QT       += core gui

win32 {
    INCLUDEPATH += c:/usr/include
    LIBS += c:/usr/lib/*.lib
}

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += QtGLib-2.0 QtGStreamer-0.10 QtGStreamerUi-0.10 gstreamer-0.10 gstreamer-base-0.10
    QMAKE_CXXFLAGS += -std=c++0x -Wno-multichar
}

dicom {
    DEFINES += WITH_DICOM
    LIBS += -ldcmnet -lwrap -li2d -ldcmdata -loflog -lofstd -lssl
    SOURCES += worklist.cpp startstudydialog.cpp dcmclient.cpp detailsdialog.cpp \
    dicomdevicesettings.cpp \
    dicomserversettings.cpp \
    dicommwlsettings.cpp \
    dicommppssettings.cpp \
    dicomstoragesettings.cpp \
    dicomserverdetails.cpp \
    worklistcolumnsettings.cpp \
    worklistquerysettings.cpp

    HEADERS += worklist.h startstudydialog.h dcmclient.h detailsdialog.h \
    dicomdevicesettings.h \
    dicomserversettings.h \
    dicommwlsettings.h \
    dicommppssettings.h \
    dicomstoragesettings.h \
    dicomserverdetails.h \
    worklistcolumnsettings.h \
    worklistquerysettings.h
}

TARGET   = beryllium
TEMPLATE = app

SOURCES += beryllium.cpp mainwindow.cpp \
    videosettings.cpp \
    settings.cpp \
    storagesettings.cpp \
    rtpsettings.cpp \
    studiessettings.cpp \
    patientdialog.cpp \
    aboutdialog.cpp \
    archivewindow.cpp
HEADERS += mainwindow.h qwaitcursor.h \
    videosettings.h \
    settings.h \
    storagesettings.h \
    rtpsettings.h \
    studiessettings.h \
    patientdialog.h \
    aboutdialog.h \
    archivewindow.h \
    product.h

FORMS   +=

RESOURCES    += beryllium.qrc
TRANSLATIONS += beryllium_ru.ts
