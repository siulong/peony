# 打桩和项目代码包含目录（需要根据项目工程位置调配）
SRC_PATH = $$PWD/../../../src
INCLUDEPATH += $$SRC_PATH
# 包含目录
include($$PWD/kt-src-gtest.pri)
include($$PWD/../test-utils.pri)

# 源文件
SOURCES += main.cpp \
           tst_uitest.cpp \

#包含的库和包
#LIBS +=  -lpeony -lgio-2.0 -lglib-2.0
#PKGCONFIG += gio-unix-2.0
