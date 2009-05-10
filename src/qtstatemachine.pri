include(../common.pri)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
DEFINES += QT_STATEMACHINE_SOLUTION
DEFINES += QT_NO_ANIMATION

qtstatemachine-uselib:!qtstatemachine-buildlib {
    LIBS += -L$$QTSTATEMACHINE_LIBDIR -l$$QTSTATEMACHINE_LIBNAME
} else {
    qtstatemachine-buildlib: QT += script gui
    include($$PWD/statemachine.pri)
}

win32 {
    contains(TEMPLATE, lib):contains(CONFIG, shared):DEFINES += Q_STATEMACHINE_CORE_EXPORT=__declspec(dllexport) Q_STATEMACHINE_GUI_EXPORT=__declspec(dllexport)
    else:qtstatemachine-uselib:DEFINES += Q_STATEMACHINE_CORE_EXPORT=__declspec(dllimport) Q_STATEMACHINE_GUI_EXPORT=__declspec(dllimport)
}
