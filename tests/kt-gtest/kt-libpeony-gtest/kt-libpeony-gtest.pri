# 打桩和项目代码包含目录（需要根据项目工程位置调配）
LIB_PEONY_PATH = $$PWD/../../../libpeony-qt
LIBS += -L$$PWD/../../../libpeony-qt/ -lpeony
# 包含目录
INCLUDEPATH += $$LIB_PEONY_PATH
include($$LIB_PEONY_PATH/../common.pri)
#include($$LIB_PEONY_PATH/libpeony-qt.pri)

QT       += core testlib gui widgets concurrent xml KWindowSystem dbus x11extras
#CONFIG += create_pc create_prl no_install_prl
CONFIG += c++11 testcase link_pkgconfig no_testcase_installs no_keywords hide_symbols lrelease
DEFINES += PEONYCORE_LIBRARY
contains(DEFINES, KY_FILE_DIALOG) {
    PKGCONFIG += kysdk-qtwidgets
}

contains(DEFINES, KY_SDK_SYSINFO) {
    PKGCONFIG += kysdk-sysinfo
}

contains(DEFINES, KY_SDK_QT_WIDGETS) {
    PKGCONFIG += kysdk-qtwidgets
}

contains(DEFINES, KY_SDK_WAYLANDHELPER) {
    PKGCONFIG += kysdk-waylandhelper
}

contains(DEFINES, KY_SDK_SYSINFO) {
    PKGCONFIG += kysdk-sysinfo
}

schemes.files += org.ukui.peony.settings.gschema.xml
schemes.path = /usr/share/glib-2.0/schemas/

PLUGIN_INSTALL_DIRS = $$[QT_INSTALL_LIBS]/peony-extensions
DEFINES += PLUGIN_INSTALL_DIRS='\\"$${PLUGIN_INSTALL_DIRS}\\"'

PROPERTIES_WINDOW_PLUGIN_INSTALL_DIRS = $$[QT_INSTALL_LIBS]/peony-properties-window
DEFINES += PROPERTIES_WINDOW_PLUGIN_INSTALL_DIRS='\\"$${PROPERTIES_WINDOW_PLUGIN_INSTALL_DIRS}\\"'

QMAKE_CXXFLAGS += -execution-charset:utf-8

#包含的库和包
LIBS +=-lgio-2.0 -lglib-2.0
PKGCONFIG += gio-unix-2.0 poppler-qt5 gsettings-qt udisks2 libnotify libcanberra openssl x11-xcb dconf

