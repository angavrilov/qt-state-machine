# -------------------------------------------------
# Project created by QtCreator 2008-12-25T19:50:44
# -------------------------------------------------

TARGET = calc
TEMPLATE = app
win32: CONFIG += console
mac:CONFIG -= app_bundle
QT = core gui script
include($$PWD/../../src/qtstatemachine.pri)

# Input
SOURCES += main.cpp calc.cpp
HEADERS += calc.h
FORMS += calc.ui
RESOURCES += calc.qrc
