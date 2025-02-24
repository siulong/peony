#include <gtest/gtest.h>
#include <qtest.h>
#include <QSignalSpy>
#include "stubext.h"
#include "file-operation/file-operation.h"
#include "file-operation/file-copy-operation.h"
using namespace testing;

class operationTest : public testing::Test
{
public:
    Peony::FileOperation* op = new Peony::FileOperation();

private:
    //打桩声明
    stub_ext::StubExt stub;

};

class copyOperationTest : public testing::Test
{
public:
    Peony::FileCopyOperation* copyOp = nullptr;
private:
    //打桩声明
    stub_ext::StubExt stub;
    QStringList sourceUris;
    QString destDirUris;
};


//测试类
TEST_F(operationTest, syncDestUriTest)
{
    QString destUri = "file:///home/k1";
    bool ret = op->syncDestUri(destUri);
    ASSERT_TRUE(ret);
}

TEST_F(copyOperationTest, copyFile)
{
    sourceUris << "file:///home/k1/1w";
    destDirUris = "file:///home/k1/test";
    copyOp = new Peony::FileCopyOperation(sourceUris, destDirUris);
    QSignalSpy spyOpFinished(copyOp, &Peony::FileOperation::operationFinished);
    ASSERT_TRUE(QThreadPool::globalInstance()->tryStart(copyOp));
    ASSERT_TRUE(spyOpFinished.isValid());
    ASSERT_TRUE(spyOpFinished.isEmpty());
    ASSERT_TRUE(spyOpFinished.wait(2000 * 10));
    const int startResult = spyOpFinished.count();
    ASSERT_EQ(startResult, 1);
}


