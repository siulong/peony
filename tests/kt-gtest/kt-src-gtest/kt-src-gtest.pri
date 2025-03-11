# 打桩和项目代码包含目录（需要根据项目工程位置调配）
LIB_PEONY_PATH = $$PWD/../../../src
# 包含目录
INCLUDEPATH += $$LIB_PEONY_PATH
include($$LIB_PEONY_PATH/../common.pri)
include($$LIB_PEONY_PATH/../libpeony-qt/libpeony-qt-header.pri)
include($$LIB_PEONY_PATH/../3rd-parties/SingleApplication/singleapplication.pri)
include($$LIB_PEONY_PATH/windows/windows.pri)
include($$LIB_PEONY_PATH/windows/windows-peony.pri)
include($$LIB_PEONY_PATH/control/control.pri)
include($$LIB_PEONY_PATH/../plugin-iface/unstable/window-plugin-iface.pri)
include($$LIB_PEONY_PATH/../plugin-iface/plugin-iface.pri)
#include($$LIB_PEONY_PATH/src.pri)
QT       += network core widgets gui concurrent xml KWindowSystem dbus x11extras  widgets

CONFIG += c++11 testcase link_pkgconfig no_testcase_installs no_keywords hide_symbols lrelease
DEFINES  += QAPPLICATION_CLASS=QApplication

contains(DEFINES, KY_SDK_KABASE) {
    PKGCONFIG += kysdk-kabase
}

contains(DEFINES, KY_SDK_DATACOLLECT) {
    PKGCONFIG += kysdk-datacollect
}

#LIBS += -L$$PWD/../libpeony-qt/ -lpeony

contains(DEFINES, KY_SDK_QT_WIDGETS) {
    PKGCONFIG += kysdk-qtwidgets
}

contains(DEFINES, KY_SDK_WAYLANDHELPER) {
    PKGCONFIG += kysdk-waylandhelper
}

HEADERS += \
    $$LIB_PEONY_PATH/peony-application.h \
    $$LIB_PEONY_PATH/peony-main-window-style.h \
    $$LIB_PEONY_PATH/main-window-factory-plugin-manager.h \

SOURCES += \
    $$LIB_PEONY_PATH/main-window-factory-plugin-manager.cpp \
    $$LIB_PEONY_PATH/peony-application.cpp \
    $$LIB_PEONY_PATH/peony-main-window-style.cpp \

WINDOW_PLUGIN_INSTALL_DIRS = $$[QT_INSTALL_LIBS]/peony-main-window
DEFINES += WINDOW_PLUGIN_INSTALL_DIRS='\\"$${WINDOW_PLUGIN_INSTALL_DIRS}\\"'

DEFINES += QT_DEPRECATED_WARNINGS

QMAKE_CXXFLAGS += -execution-charset:utf-8

#包含的库和包
LIBS +=-lgio-2.0 -lglib-2.0 -lX11 -lukui-log4qt -L$$PWD/../../../libpeony-qt/ -lpeony
PKGCONFIG +=gio-2.0 glib-2.0 gio-unix-2.0 gsettings-qt libcanberra libnotify udisks2 openssl dconf polkit-gobject-1
