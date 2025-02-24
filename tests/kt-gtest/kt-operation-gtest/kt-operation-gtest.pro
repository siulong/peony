include($$PWD/../test-utils.pri)

QT       += core testlib gui widgets
TEMPLATE = app
CONFIG += c++11 testcase link_pkgconfig no_testcase_installs no_keywords hide_symbols

# 打桩和项目代码包含目录（需要根据项目工程位置调配）
LIB_PEONY_PATH = $$PWD/../../../libpeony-qt
# 包含目录
INCLUDEPATH += $$LIB_PEONY_PATH
HEADERS += $$files($$LIB_PEONY_PATH/file-operation/file-operation-progress-bar-helper.h) \

# 源文件
SOURCES += main.cpp \
           tst-operation-progress-bar-gtest.cpp \
           tst_operationtest.cpp \
           $$files($$LIB_PEONY_PATH/file-operation/file-operation-progress-bar-helper.cpp)

#包含的库和包
LIBS +=  -lpeony -lgio-2.0 -lglib-2.0
PKGCONFIG += gio-unix-2.0
