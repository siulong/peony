/*
 * Peony-Qt
 *
 * Copyright (C) 2024, KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Yue Lan <lanyue@kylinos.cn>
 * Authors: Meihong He <hemeihong@kylinos.cn>
 *
 */


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


