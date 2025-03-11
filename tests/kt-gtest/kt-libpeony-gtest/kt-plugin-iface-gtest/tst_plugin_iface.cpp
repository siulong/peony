#include <gtest/gtest.h>
#include <qtest.h>
#include "stubext.h"

using namespace testing;

class pluginIfaceTest : public testing::Test
{
public:

private:
    //打桩声明
    stub_ext::StubExt stub;

};




