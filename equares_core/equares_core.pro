#-------------------------------------------------
#
# Project created by QtCreator 2013-11-17T18:20:38
#
#-------------------------------------------------

!include (../equares.pri):error("Couldn't find the equares.pri file!")

#QT       -= gui
QT       += script

TARGET = equares_core
TEMPLATE = lib
DEFINES += EQUARES_CORE_LIBRARY
# DEFINES += EQUARES_DUMP_SIMULATION_LOG

contains(DEFINES, EQUARES_CORE_STATIC) {
    CONFIG += staticlib
}

!isEmpty(ACML_DIR) {
    INCLUDEPATH += $$ACML_DIR/include
    DEFINES += WITH_ACML
    unix: LIBS += -L$$ACML_DIR/lib -lacml
    win32: LIBS += -L$$ACML_DIR/lib -llibacml_dll
}

SOURCES += equares_core.cpp \
    ConstantSourceBox.cpp \
    PendulumBox.cpp \
    Rk4Box.cpp \
    DumpBox.cpp \
    Rk4AdjustParamBox.cpp \
    ProjectionBox.cpp \
    CrossSectionBox.cpp \
    ValveBox.cpp \
    CountedFilterBox.cpp \
    CanvasBox.cpp \
    DoublePendulumBox.cpp \
    equares_script.cpp \
    BitmapBox.cpp \
    IntervalFilterBox.cpp \
    MathieuBox.cpp \
    VibratingPendulumBox.cpp \
    OdeJsBox.cpp \
    initBoxFactory.cpp \
    OdeCxxBox.cpp \
    equares_exec.cpp \
    BoxSettings.cpp \
    PerTypeStorage.cpp \
    check_lib.cpp \
    GridGeneratorBox.cpp \
    ParamArrayBox.cpp \
    InterpolatorBox.cpp \
    LinOdeStabCheckerBox.cpp \
    PointInputBox.cpp \
    SimpleInputBox.cpp \
    RangeInputBox.cpp \
    SignalInputBox.cpp \
    DataInputBox.cpp \
    MergeBox.cpp \
    box_util.cpp \
    ReplicatorBox.cpp \
    CxxBuildHelper.cpp \
    FdeCxxBox.cpp \
    FdeIteratorBox.cpp \
    JoinBox.cpp \
    ScalarizeBox.cpp \
    ThresholdDetectorBox.cpp \
    PauseBox.cpp \
    DifferentiateBox.cpp \
    RectInputBox.cpp \
    CounterBox.cpp \
    SplitBox.cpp \
    math_util.cpp \
    CxxTransformBox.cpp \
    EigenvaluesBox.cpp

HEADERS += equares_core.h\
        equares_core_global.h \
    ConstantSourceBox.h \
    PendulumBox.h \
    Rk4Box.h \
    DumpBox.h \
    Rk4AdjustParamBox.h \
    ProjectionBox.h \
    CrossSectionBox.h \
    ValveBox.h \
    CountedFilterBox.h \
    CanvasBox.h \
    DoublePendulumBox.h \
    equares_script.h \
    BitmapBox.h \
    IntervalFilterBox.h \
    MathieuBox.h \
    OdeBox.h \
    VibratingPendulumBox.h \
    OdeJsBox.h \
    script_arrays.h \
    initBoxFactory.h \
    OdeCxxBox.h \
    EquaresException.h \
    equares_common.h \
    equares_exec.h \
    PerTypeStorage.h \
    check_lib.h \
    GridGeneratorBox.h \
    ParamArrayBox.h \
    InterpolatorBox.h \
    LinOdeStabCheckerBox.h \
    PointInputBox.h \
    SimpleInputBox.h \
    RangeInputBox.h \
    SignalInputBox.h \
    DataInputBox.h \
    MergeBox.h \
    box_util.h \
    ReplicatorBox.h \
    CxxBuildHelper.h \
    FdeCxxBox.h \
    FdeIteratorBox.h \
    JoinBox.h \
    ScalarizeBox.h \
    ThresholdDetectorBox.h \
    PauseBox.h \
    DifferentiateBox.h \
    RectInputBox.h \
    CounterBox.h \
    SplitBox.h \
    math_util.h \
    CxxTransformBox.h \
    EigenvaluesBox.h

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE061BE2F
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = equares_core.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

RESOURCES += \
    equares_core.qrc

contains(QMAKESPEC, ^.*msvc.*$) {
    MAKE_COMMAND = nmake
} else {
    MAKE_COMMAND = make
}

equaresbuildspecs.output = $$DESTDIR/buildpath.txt $$DESTDIR/makecmd.txt
equaresbuildspecs.commands = \
    echo $$(PATH)>$$DESTDIR/buildpath.txt $$escape_expand(\\n\\t) \
    echo $$(INCLUDE)>$$DESTDIR/buildinclude.txt $$escape_expand(\\n\\t) \
    echo $$(LIB)>$$DESTDIR/buildlib.txt $$escape_expand(\\n\\t) \
    echo $$MAKE_COMMAND>$$DESTDIR/makecmd.txt

QMAKE_EXTRA_TARGETS += equaresbuildspecs
POST_TARGETDEPS += equaresbuildspecs
