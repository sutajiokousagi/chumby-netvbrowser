
QT       += core gui webkit network

TARGET   = NeTVBrowser
TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           mywebview.cpp \
           mainwindow_comm.cpp \
           mainwindow_common.cpp
HEADERS += mainwindow.h mywebview.h
FORMS   += mainwindow.ui

# Singleton & command line argument passing
include(./qtsingleapplication-2.6_1-opensource/src/qtsingleapplication.pri)

# Testing socket class
include(./bfSocketServer/src/bfSocketserver.pri)

# Changes the name of the target, when is debug mode
CONFIG( debug, debug|release ) {
    TARGET = $${TARGET}_debug
    BUILD_NAME = debug
}
CONFIG( release, debug|release ) {
    TARGET = $${TARGET}
    BUILD_NAME = release
}

# Temporary folders for the auxiliar files
INCLUDEPATH += $$PWD/tmp/$$BUILD_NAME $$PWD/tmp
OBJECTS_DIR = $$PWD/tmp/$$BUILD_NAME
MOC_DIR = $$PWD/tmp/$$BUILD_NAME
UI_DIR = $$PWD/tmp/$$BUILD_NAME
RCC_DIR = $$PWD/tmp/$$BUILD_NAME
DESTDIR = $$PWD/bin
