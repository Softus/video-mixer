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
#    LIBS += -ldcmnet -lwrap -li2d -ldcmdata -ldcmtls -loflog -lofstd -lssl
     LIBS += -L/usr/lib/dcmtk -ldcmnet -llibi2d -ldcmdata -ldcmtls -loflog -lofstd -lssl
    SOURCES += worklist.cpp startstudydialog.cpp dcmclient.cpp detailsdialog.cpp
    HEADERS += worklist.h startstudydialog.h dcmclient.h detailsdialog.h
}

TARGET   = beryllium
TEMPLATE = app

SOURCES += beryllium.cpp mainwindow.cpp basewidget.cpp
HEADERS += mainwindow.h basewidget.h qwaitcursor.h



FORMS   +=

RESOURCES    += beryllium.qrc
TRANSLATIONS += beryllium_ru.ts
