#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "stubext.h"
#include "global-settings.h"

using namespace testing;

class OperationMenu : public testing::Test {
public:
    Peony::GlobalSettings* settings = Peony::GlobalSettings::getInstance();

private:
    stub_ext::StubExt stub;
};

//TEST(tst, tst)
//{
//    EXPECT_EQ(1, 1);
//    ASSERT_THAT(0, Eq(0));
//}

TEST_F(OperationMenu, setNetwork)
{
    bool flag = false;
    settings->setValue(SHOW_NETWORK, flag);
    EXPECT_EQ(settings->getValue(SHOW_NETWORK).toBool(), flag);
}
