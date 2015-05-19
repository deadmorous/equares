QT -= gui
TARGET = ode
TEMPLATE = lib
CONFIG += dll
QMAKE_CXXFLAGS += -O2
SOURCES += ode.cpp
INCLUDES += ../../../../public/include

#contains(QMAKESPEC, ^.*msvc.*$) {
windows {
  WINDESTTARGET = $$OUT_PWD/release/ode.dll
  WINDESTTARGET = $$replace(WINDESTTARGET, /, \\)
  WINDESTFINAL = $$OUT_PWD/
  WINDESTFINAL = $$replace(WINDESTFINAL, /, \\)
  QMAKE_POST_LINK += xcopy /Y $$WINDESTTARGET $$WINDESTFINAL
} else {
  DESTDIR = $$OUT_PWD
}
