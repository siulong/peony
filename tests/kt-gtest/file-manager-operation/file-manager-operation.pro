include($$PWD/../test-utils.pri)
QT       += core testlib gui widgets
TEMPLATE = app
CONFIG += c++11 testcase link_pkgconfig no_testcase_installs

LIB_PEONY_PATH = $$PWD/../../../libpeony-qt

INCLUDEPATH += $$LIB_PEONY_PATH           \
               $$LIB_PEONY_PATH/controls/directory-view \
               $$LIB_PEONY_PATH/controls/directory-view/directory-view-factory \
               $$LIB_PEONY_PATH/model \
               $$LIB_PEONY_PATH/controls/directory-view-factory \

SOURCES += main.cpp \
           file-manager-operation.cpp

LIBS +=  -lpeony -lgio-2.0 -lglib-2.0
PKGCONFIG += gio-unix-2.0 gsettings-qt
