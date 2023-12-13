#-------------------------------------------------
#
# Project created by QtCreator 2023-11-29T09:45:20
#
#-------------------------------------------------

QT       += network core widgets gui concurrent xml KWindowSystem dbus x11extras

#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS += -Werror=return-type -Werror=return-local-addr -Werror=uninitialized -Werror=unused-label
include(../../../common.pri)
include(../../../plugin-iface/unstable/window-plugin-iface.pri)
include(controls/property-page.pri)
INCLUDEPATH += $$PWD/../../../libpeony-qt/file-operation
INCLUDEPATH += $$PWD/../../../libpeony-qt/file-launcher
INCLUDEPATH += $$PWD/../../../libpeony-qt/model
INCLUDEPATH += $$PWD/../../../libpeony-qt/vfs
INCLUDEPATH += $$PWD/../../../libpeony-qt/convenient-utils
INCLUDEPATH += $$PWD/../../../libpeony-qt/convenient-utils/disc
INCLUDEPATH += $$PWD/../../../libpeony-qt/effects
INCLUDEPATH += $$PWD/../../../plugin-iface/
INCLUDEPATH += $$PWD/../../../libpeony-qt/
INCLUDEPATH += $$PWD/../../../libpeony-qt/thumbnail


DISTFILES += $$PWD/common.json

PKGCONFIG +=gio-2.0 glib-2.0 gio-unix-2.0 poppler-qt5 gsettings-qt libcanberra libnotify udisks2 openssl dconf
LIBS +=-L$$PWD/../../../  -lpeony -lX11 -lukui-log4qt

CONFIG += debug link_pkgconfig plugin no_keywords
TARGET = stable-properties-window
TEMPLATE = lib
DEFINES += STABLEPROPERTIESWINDOW_LIBRARY

CONFIG += c++11

#contains(DEFINES, KY_FILE_DIALOG) {
#    PKGCONFIG += kysdk-qtwidgets
#}

#contains(DEFINES, KY_SDK_SYSINFO) {
#    PKGCONFIG += kysdk-sysinfo
#}

#contains(DEFINES, KY_SDK_QT_WIDGETS) {
#    PKGCONFIG += kysdk-qtwidgets
#}

#contains(DEFINES, KY_SDK_WAYLANDHELPER) {
#    PKGCONFIG += kysdk-waylandhelper
#}

#contains(DEFINES, KY_SDK_SYSINFO) {
#    PKGCONFIG += kysdk-sysinfo
#}

#schemes.files += org.ukui.peony.settings.gschema.xml
#schemes.path = /usr/share/glib-2.0/schemas/

PROPERTIES_WINDOW_PLUGIN_INSTALL_DIRS = $$[QT_INSTALL_LIBS]/peony-properties-window
DEFINES += PROPERTIES_WINDOW_PLUGIN_INSTALL_DIRS='\\"$${PROPERTIES_WINDOW_PLUGIN_INSTALL_DIRS}\\"'

#QMAKE_CXXFLAGS += -execution-charset:utf-8

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    properties-window-factory.cpp \
    properties-window.cpp

HEADERS += \
    properties-window.h \
    properties-window_global.h \
    properties-window-factory.h \

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_LIBS]/peony-properties-window
}
!isEmpty(target.path): INSTALLS += target
