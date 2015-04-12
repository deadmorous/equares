#-------------------------------------------------
#
# Project created by QtCreator 2013-11-17T18:22:42
#
#-------------------------------------------------

!include (../equares.pri):error("Couldn't find the equares.pri file!")

QT       += core gui script

contains(QT_VERSION, ^5\\..*) : QT += widgets

TARGET = equares_gui
TEMPLATE = app


SOURCES += main.cpp\
    MainWindow.cpp \
    SimVisualizer.cpp \
    Animator.cpp

HEADERS  += MainWindow.h \
    sim_types.h \
    SimVisualizer.h \
    Animator.h

FORMS    += MainWindow.ui

LIBS += -lequares_core
