#-------------------------------------------------
#
# Project cronica
#
#-------------------------------------------------

QT       += core gui

win32 {
    INCLUDEPATH += c:/usr/include
    LIBS += c:/usr/lib/*.lib
}

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += QtGStreamer-0.10 QtGStreamerUi-0.10
    QMAKE_CXXFLAGS += -std=c++11
}

TARGET   = cronica
TEMPLATE = app

SOURCES += cronica.cpp mainwindow.cpp
HEADERS += mainwindow.h

FORMS   +=

RESOURCES    += cronica.qrc
TRANSLATIONS += cronica_ru.ts
