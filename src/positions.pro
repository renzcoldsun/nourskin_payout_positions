QT       += core

QT       -= gui
QT       += sql

TARGET = positions
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp task.cpp

HEADERS += task.h

