include($$PWD/../test-utils.pri)
QT       += core testlib gui widgets dbus concurrent KWindowSystem KWaylandClient
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = app
CONFIG += c++11 testcase link_pkgconfig no_testcase_installs
CONFIG += no_keywords

include($$PWD/../../../common.pri)
include($$PWD/../../../libpeony-qt/libpeony-qt-header.pri)
include($$PWD/../../../3rd-parties/SingleApplication/singleapplication.pri)
include($$PWD/../../../3rd-parties/qtsingleapplication/qtsingleapplication.pri)
LIB_PEONY_PATH = $$PWD/../../../libpeony-qt
DESKTOP_PEONY_PATH = $$PWD/../../../peony-qt-desktop

INCLUDEPATH += $$LIB_PEONY_PATH

SOURCES += main.cpp \
           tst_desktoptest.cpp
#包含的库和包
LIBS +=  -lpeony -lX11
PKGCONFIG += gio-unix-2.0 gio-2.0 glib-2.0 gsettings-qt libcanberra wayland-client dconf
