# -------------------------------------------------
# Project created by QtCreator 2008-12-16T16:32:05
# -------------------------------------------------
QT += script
DEPENDPATH += .
INCLUDEPATH += .
TARGET = blackjack
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp
include($$PWD/../../src/qtstatemachine.pri)
FORMS += blackjack.ui
RESOURCES += bj.qrc
