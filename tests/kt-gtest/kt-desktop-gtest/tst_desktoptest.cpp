#include <gtest/gtest.h>
#include <QWidget>
#include "stubext.h"
#include "global-settings.h"

using namespace testing;

class DesktopItemProxyModelTest : public testing::Test
{
public:

private:
    //打桩声明
    stub_ext::StubExt stub;

};
class GlobalSettingsTest : public testing::Test
{
public:
    Peony::GlobalSettings* settings = Peony::GlobalSettings::getInstance();
private:
    //打桩声明
    stub_ext::StubExt stub;

};
