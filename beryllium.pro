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
    PKGCONFIG += QtGLib-2.0 QtGStreamer-0.10 QtGStreamerUi-0.10 gstreamer-0.10
    QMAKE_CXXFLAGS += -std=c++11
}

dicom {
    DEFINES += WITH_DICOM
    LIBS += -ldcmnet -lwrap  -ldcmdata -ldcmtls -loflog -lofstd
    SOURCES += worklist.cpp dcmassoc.cpp
    HEADERS += worklist.h dcmassoc.h
}

TARGET   = beryllium
TEMPLATE = app

SOURCES += beryllium.cpp mainwindow.cpp basewidget.cpp
HEADERS += mainwindow.h basewidget.h

FORMS   +=

RESOURCES    += beryllium.qrc
TRANSLATIONS += beryllium_ru.ts
