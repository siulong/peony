#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QDebug>
#include <QObject>
#include "stubext.h"

#include "model/file-item-model.h"
#include "model/file-item-proxy-filter-sort-model.h"

using namespace testing;

class FileItemModelTest : public testing::Test
{
public:
    Peony::FileItemModel* fileItemModel = new Peony::FileItemModel;
private:
    //打桩声明
    stub_ext::StubExt stub;
};

//测试集
//TEST(tst, tst)
//{
//}


//测试类
TEST_F(FileItemModelTest, columnCount)
{
    /* 该函数用于验证搜索页面列数（有5列，区别其它常规目录） */
    fileItemModel->setRootUri("search:///search_uris=file:///home/kylin/test&name_regexp=demo&recursive=1&search_engine=1");
    int columnCount = fileItemModel->columnCount(QModelIndex());
    EXPECT_EQ(columnCount, 5);
}

TEST_F(FileItemModelTest, headerData)
{
    /* 该函数用于验证搜索页面列头是否有文件路径列和其名称 */
    fileItemModel->setRootUri("search:///search_uris=file:///home/kylin/test&name_regexp=demo&recursive=1&search_engine=1");
    QString headerColumeName = fileItemModel->headerData(4, Qt::Orientation::Horizontal, Qt::DisplayRole).toString();
    EXPECT_EQ(headerColumeName, QObject::tr("Path"));
}


