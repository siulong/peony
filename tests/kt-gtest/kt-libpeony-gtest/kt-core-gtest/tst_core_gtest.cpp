#include <gtest/gtest.h>
#include <qtest.h>
#include "stubext.h"

#include "file-info.h"
#include "file-info-job.h"
#include "file-infos-job.h"
#include "file-enumerator.h"
#include "file-watcher.h"

#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QSignalSpy>

using namespace testing;

class coreTest : public testing::Test
{
public:

private:
    //打桩声明
    stub_ext::StubExt stub;

};

using namespace Peony;

TEST_F(coreTest, testFileInfo)
{
    auto info1 = FileInfo::fromUri("file:///");
    auto info2 = FileInfo::fromPath("/");
    g_autoptr (GFile) tmp_file = g_file_new_for_path("/");
    auto info3 = FileInfo::fromGFile(tmp_file);

    ASSERT_EQ(info1, info2);
    ASSERT_EQ(info1, info3);
    ASSERT_EQ(info2, info3);
}

TEST_F(coreTest, testFileInfoJobSync)
{
    auto info = FileInfo::fromUri("file:///");

    FileInfoJob job("file:///");
    job.querySync();

    std::vector<std::shared_ptr<FileInfo> >infos;
    infos.push_back(info);
    FileInfosJob jobs(infos);
    jobs.batchQuerySync();

    ASSERT_TRUE(!info->isEmptyInfo());

    auto tmp = FileInfo::fromUri("file:///");
    ASSERT_EQ(info, tmp);
}

TEST_F(coreTest, testFileEnumeratorSync)
{
    FileEnumerator e;
    e.setEnumerateDirectory("file:///");
    e.setEnumerateWithInfoJob();
    e.enumerateSync();

    auto files = e.getChildrenUris();
    ASSERT_TRUE(!files.isEmpty());
}

TEST_F(coreTest, testFileWatcher)
{
    FileWatcher w("file:///tmp");
    w.setMonitorChildrenChange();
    w.startMonitor();
    QSignalSpy spy(&w, &FileWatcher::fileCreated);
    QDir dir("/tmp");
    if (!dir.mkdir("peony-core-test")) {
        return;
    }
    spy.wait();
    dir.remove("peony-core-test");
    ASSERT_TRUE(spy.isValid());
}
