#-------------------------------------------------
#
# Project created by QtCreator 2013-11-17T18:22:08
#
#-------------------------------------------------

!include (../equares.pri):error("Couldn't find the equares.pri file!")

QT       += core script
#QT       -= gui

TARGET = equares_con
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp

LIBS += -lequares_core
