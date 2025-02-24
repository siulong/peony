#include <gtest/gtest.h>

//#include <QSignalSpy>
#include <QStandardPaths>
#include "stubext.h"
#include "../src/windows/main-window-factory.h"
#include "../src/control/navigation-side-bar.h"
using namespace testing;

class uiTest : public testing::Test
{
public:
    MainWindowFactory* factory = new MainWindowFactory;

private:
    //打桩声明
    stub_ext::StubExt stub;

};

class sideTest : public testing::Test
{
public:
    NavigationSideBar* side = new NavigationSideBar;
private:
    //打桩声明
    stub_ext::StubExt stub;
    QStringList sourceUris;
    QString destDirUris;
};

TEST_F(uiTest, createWindow)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QWidget *window = factory->createWindow(path);
    window->show();
}

TEST_F(uiTest, createWindow2)
{
    QStringList uris;
    uris<<QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    uris<<QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QWidget *window2 = factory->createWindow(uris);
    window2->show();
}


