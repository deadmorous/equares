QT -= gui
TARGET = fde
TEMPLATE = lib
CONFIG += dll
QMAKE_CXXFLAGS += -O2
SOURCES += fde.cpp
INCLUDEPATH += ../../../../public/include

#contains(QMAKESPEC, ^.*msvc.*$) {
windows {
  WINDESTTARGET = $$OUT_PWD/release/fde.dll
  WINDESTTARGET = $$replace(WINDESTTARGET, /, \\)
  WINDESTFINAL = $$OUT_PWD/
  WINDESTFINAL = $$replace(WINDESTFINAL, /, \\)
  QMAKE_POST_LINK += xcopy /Y $$WINDESTTARGET $$WINDESTFINAL
} else {
  DESTDIR = $$OUT_PWD
}
