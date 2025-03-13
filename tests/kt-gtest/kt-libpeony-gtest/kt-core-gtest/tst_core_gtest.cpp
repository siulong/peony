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
