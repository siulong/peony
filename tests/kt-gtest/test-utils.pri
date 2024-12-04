# test-utils.pri

CONFIG += console c++11 link_pkgconfig
CONFIG += thread

TEST_UTILS_PATH = $$_PRO_FILE_PWD_/../kt-test-utils
INCLUDEPATH += $$TEST_UTILS_PATH/cpp-stub \
               $$TEST_UTILS_PATH/stub-ext \

HEADERS += $$files($$TEST_UTILS_PATH/cpp-stub/*.h) \
           $$files($$TEST_UTILS_PATH/cpp-stub/*.hpp) \
           $$files($$TEST_UTILS_PATH/stub-ext/*.h)

SOURCES += $$files($$TEST_UTILS_PATH/cpp-stub/*.cpp) \
           $$files($$TEST_UTILS_PATH/stub-ext/*.cpp)

# 覆盖率
QMAKE_LFLAGS +=-fprofile-arcs -ftest-coverage
QMAKE_CXXFLAGS += --coverage -fno-inline -fno-access-control -fno-exceptions

LIBS += -lgtest -lgcov
