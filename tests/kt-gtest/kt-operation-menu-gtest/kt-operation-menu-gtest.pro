include($$PWD/../test-utils.pri)
QT       += core testlib gui widgets dbus
TEMPLATE = app
CONFIG += c++11 testcase link_pkgconfig no_testcase_installs

LIB_PEONY_PATH = $$PWD/../../../libpeony-qt

INCLUDEPATH += $$LIB_PEONY_PATH

SOURCES += main.cpp \
           tst_operation-menu-test.cpp

LIBS +=  -lpeony -lgio-2.0 -lglib-2.0
PKGCONFIG += gio-unix-2.0
