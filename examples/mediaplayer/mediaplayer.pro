TEMPLATE = app
TARGET = scxmlplayer
QT += script \
    sql \
    phonon
include($$PWD/../../src/qtstatemachine.pri)

HEADERS += spmodel.h \
    spengine.h \
    spview.h \
    spharvester.h \
    songdata.h
SOURCES += main.cpp \
    spmodel.cpp \
    spengine.cpp \
    spview.cpp \
    spharvester.cpp
FORMS += mediaplayer.ui
RESOURCES += mediaplayer.qrc
win32:CONFIG += console
mac:CONFIG -= app_bundle
INCLUDEPATH += .
DEPENDPATH += .
