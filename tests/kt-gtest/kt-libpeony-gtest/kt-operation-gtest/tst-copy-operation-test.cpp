#include <gtest/gtest.h>
#include <qtest.h>
#include <QSignalSpy>
#include "stubext.h"
#include "file-operation/file-copy-operation.h"
using namespace testing;

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

TEST_F(copyOperationTest, copyFile)
{

}
