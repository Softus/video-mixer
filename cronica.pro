#-------------------------------------------------
#
# Project cronica
#
#-------------------------------------------------

QT       += core gui

CONFIG += link_pkgconfig
PKGCONFIG += QtGStreamer-0.10 QtGStreamerUi-0.10

TARGET = cronica
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    +=

RESOURCES += \
    res.qrc

TRANSLATIONS += cronica_ru.ts
