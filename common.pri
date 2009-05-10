infile(config.pri, SOLUTIONS_LIBRARY, yes): CONFIG += qtstatemachine-uselib
TEMPLATE += fakelib
QTSTATEMACHINE_LIBNAME = $$qtLibraryTarget(QtSolutions_StateMachineFramework-1.1)
TEMPLATE -= fakelib
QTSTATEMACHINE_LIBDIR = $$PWD/lib
unix:qtstatemachine-uselib:!qtstatemachine-buildlib:QMAKE_RPATHDIR += $$QTSTATEMACHINE_LIBDIR
