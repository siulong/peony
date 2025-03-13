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
#include <qtest.h>
#include "stubext.h"

using namespace testing;

class modelTest : public testing::Test
{
public:

private:
    //打桩声明
    stub_ext::StubExt stub;
};


#include "model/file-item-model.h"
#include "global-settings.h"
class FileItemModelTest : public testing::Test
{
public:
    Peony::FileItemModel* fileItemModel = new Peony::FileItemModel;
private:
    //打桩声明
    stub_ext::StubExt stub;
};

//测试类
TEST_F(FileItemModelTest, columnCount)
{
    /* 该函数用于验证列表视图页面列数（常规目录有四列，回收站有5列） */
    fileItemModel->setRootUri("file:///home/kylin");
    int columnCount = fileItemModel->columnCount(QModelIndex());
    EXPECT_EQ(columnCount, 4);

    fileItemModel->setRootUri("trash:///");
    columnCount = fileItemModel->columnCount(QModelIndex());
    EXPECT_EQ(columnCount, 5);
}

TEST_F(FileItemModelTest, headerData)
{
     /* 该函数用于验证列表视图页面列头的各列名称 */
    fileItemModel->setRootUri("file:///home/kylin");
    QString headerColumeName = fileItemModel->headerData(0, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
    EXPECT_EQ(headerColumeName, QObject::tr("File Name"));
    if(Peony::GlobalSettings::getInstance()->getValue(SHOW_CREATE_TIME).toBool()){
        headerColumeName = fileItemModel->headerData(1, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
        EXPECT_EQ(headerColumeName, QObject::tr("Create Date"));
    }else{
        headerColumeName = fileItemModel->headerData(1, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
        EXPECT_EQ(headerColumeName, QObject::tr("Modified Date"));
    }
    headerColumeName = fileItemModel->headerData(2, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
    EXPECT_EQ(headerColumeName, QObject::tr("File Type"));
    headerColumeName = fileItemModel->headerData(3, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
    EXPECT_EQ(headerColumeName, QObject::tr("File Size"));

    fileItemModel->setRootUri("trash:///");
    headerColumeName = fileItemModel->headerData(0, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
    EXPECT_EQ(headerColumeName, QObject::tr("File Name"));
    headerColumeName = fileItemModel->headerData(1, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
    EXPECT_EQ(headerColumeName, QObject::tr("Delete Date"));
    headerColumeName = fileItemModel->headerData(2, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
    EXPECT_EQ(headerColumeName, QObject::tr("File Type"));
    headerColumeName = fileItemModel->headerData(3, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
    EXPECT_EQ(headerColumeName, QObject::tr("File Size"));
    headerColumeName = fileItemModel->headerData(4, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
    EXPECT_EQ(headerColumeName, QObject::tr("Original Path"));

}


#include "side-bar-file-system-item.h"
#include "side-bar-model.h"
class SideBarModelTest : public testing::Test
{
public:
    Peony::SideBarModel* sideBarModel = new Peony::SideBarModel;
private:
    //打桩声明
    stub_ext::StubExt stub;
};
TEST_F(SideBarModelTest, computerFindChildren){
    Peony::SideBarFileSystemItem *computerItem = new Peony::SideBarFileSystemItem(nullptr, nullptr, nullptr, sideBarModel);
    computerItem->findChildren();
    auto children = computerItem->m_children;
    for (auto item : *children) {
        bool bFileSystem = item->m_uri == "file:///";
        if (item->m_uri == "file:///") {/* 侧边栏计算机项的子项存在文件系统 */
            ASSERT_TRUE(item->m_mountPoint == "/");
            ASSERT_TRUE(item->m_mounted == true);
            EXPECT_EQ(item->m_displayName, QObject::tr("File System"));
            return;
        }
    }
}

