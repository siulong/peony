#include <gtest/gtest.h>
#include "stubext.h"
#include "file-utils.h"
#include "convenient-utils/clipboard-utils.h"
#include "file-operation/file-operation-error-dialogs.h"
#include "file-operation/file-operation-error-handler.h"

using namespace testing;

class operationTest : public testing::Test
{
public:
    Peony::ClipboardUtils* clipboard = Peony::ClipboardUtils::getInstance();
private:
    //打桩声明
    stub_ext::StubExt stub;

};
//声明一个测试类，用于初始化FileOperationErrorDialogWarning
class operationErrorDialogsTest : public testing::Test
{
public:
    Peony::FileOperationErrorDialogWarning* dlg = new Peony::FileOperationErrorDialogWarning;
    Peony::FileOperationError err;
private:
    stub_ext::StubExt stub;
};

//测试集
TEST(tst, tst)
{
    //断言/data目录权限为非只读
    EXPECT_FALSE(Peony::FileUtils::isReadonly("/data"));
}
//测试类
TEST_F(operationTest, utils)
{
    //验证文件管理器剪切版功能
    QStringList list;
    QString uri = "file:///data/file.txt";
    list << uri;
    //设置文件uri
    clipboard->setClipboardFiles(list, false);
    //获取剪切版设置内容
    QString firstUri = clipboard->getClipboardFilesUris().first();
    //断言获取uri与设置的是否相同
    EXPECT_EQ(firstUri.toStdString(), "file:///data/file.txt");
}
//task206350，测试全部应用功能
TEST_F(operationErrorDialogsTest, ignoreAllSameError)
{
    //这里主要是判断一下初始化的相关值，实际上可以不用
    bool dosame = dlg->property("m_do_same").toBool();
    EXPECT_FALSE(dosame);
    bool same = dlg->m_do_same;
    //模拟传入了一个err信息，需要界面处理，此处会进行弹窗选择
    //弹窗主要用于测试QCheckBox的显示是否正常已经操作交互
    dlg->handle(err);
    //根据选择的结果进行测试，这里用于判断想要测试的功能，根据task需求，勾选全部应用，需要err返回一个ignoreAll，则测试成功
    EXPECT_EQ(err.respCode, Peony::IgnoreAll);
}
