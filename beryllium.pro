#-------------------------------------------------
#
# Project beryllium
#
#-------------------------------------------------

QT       += core gui

# GCC tuning
*-g++*:QMAKE_CXXFLAGS += -std=c++0x -Wno-multichar

win32 {
    INCLUDEPATH += c:/usr/include
    LIBS += c:/usr/lib/*.lib advapi32.lib netapi32.lib wsock32.lib
}

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += QtGLib-2.0 QtGStreamer-0.10 QtGStreamerUi-0.10 gstreamer-0.10 gstreamer-base-0.10
}

dicom {
    QT       += network
    DEFINES  += WITH_DICOM
    LIBS     += -ldcmnet -lwrap -li2d -ldcmdata -loflog -lofstd -lssl

    SOURCES  += worklist.cpp dcmclient.cpp detailsdialog.cpp \
    dicomdevicesettings.cpp \
    dicomserversettings.cpp \
    dicomstoragesettings.cpp \
    dicomserverdetails.cpp \
    worklistcolumnsettings.cpp \
    worklistquerysettings.cpp

    HEADERS += worklist.h dcmclient.h detailsdialog.h \
    dicomdevicesettings.h \
    dicomserversettings.h \
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
    studiessettings.cpp \
    patientdialog.cpp \
    aboutdialog.cpp \
    archivewindow.cpp \
    dicommppsmwlsettings.cpp \
    physicianssettings.cpp
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
    physicianssettings.h

FORMS   +=

RESOURCES    += beryllium.qrc
TRANSLATIONS += beryllium_ru.ts
