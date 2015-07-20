QT       += core
QT       -= gui

# GCC tuning
*-g++*:QMAKE_CXXFLAGS += -std=c++0x -Wno-multichar

CONFIG += link_pkgconfig
greaterThan(QT_MAJOR_VERSION, 4) {
    PKGCONFIG += Qt5GLib-2.0 Qt5GStreamer-1.0 Qt5GStreamerUi-1.0
}
else {
    PKGCONFIG += QtGLib-2.0 QtGStreamer-0.10 QtGStreamerUi-0.10
}

TARGET    = videomixer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += src/main.cpp \
    src/mixer.cpp

HEADERS += \
    src/mixer.h
