#include "gio/gio.h"
#include <gtest/gtest.h>
#include <qtest.h>
#include <QStandardPaths>
#include <QWidget>
#include "stubext.h"
#include "file-operation/file-operation.h"
#include "file-operation/file-operation-manager.h"
#include "file-operation/file-operation-progress-bar-helper.h"
using namespace testing;

class operationProgressBarTest : public testing::Test
{
public:

private:
    //打桩声明
    stub_ext::StubExt stub;

};

TEST_F(operationProgressBarTest, timeToStringTest)
{
    int time = 3600;
//    progressBar->m_main_progressbar->timeToString(time);
    QString stringTime = progressBarHelper::timeToString(time);
    ASSERT_EQ(stringTime.toStdString(), "01hrs00mins00sec");
}

TEST_F(operationProgressBarTest, calculateSpeed)
{
    qint64 size = 300 * 1024 * 1024;
    double elapsedSeconds = 2.5;
    double speed = progressBarHelper::calculateSpeed(size, elapsedSeconds);
    ASSERT_EQ(speed, 120.00);
}

TEST_F(operationProgressBarTest, calculateEstimatedTime)
{
    qint64 size = 300 * 1024 * 1024;
    double speed = 15.00;
    int time = progressBarHelper::calculateEstimatedTime(size,speed);
    ASSERT_EQ(time, 20);
}
