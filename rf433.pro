#-------------------------------------------------
#
# Project created by QtCreator 2014-06-08T16:20:44
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = rf433
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

target.path = /usr/bin/
INSTALLS += target


SOURCES += main.cpp \
    radio433receiver.cpp \
    core.cpp \
    radio433trasmitter.cpp \
    gpio.cpp \
    radio433.cpp

HEADERS += \
    radio433receiver.h \
    core.h \
    radio433trasmitter.h \
    gpio.h \
    radio433.h
