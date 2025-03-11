/*
 * Peony-Qt
 *
 * Copyright (C) 2024, Tianjin KYLIN Information Technology Co., Ltd.
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
 * @author: Renyg <renyangguang@kylinos.cn>
 * @date:   2024-08-23 14:52
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

#include <QDebug>
#include <QDir>
#include <QSignalSpy>

#include "directory-view-container.h"
#include "directory-view-factory-manager.h"
#include "file-info.h"
#include "file-utils.h"


using namespace testing;
using namespace Peony;

// custom operator<< function prints a QString
std::ostream& operator<<(std::ostream& os, const QString& str) {
    os << str.toStdString();
    return os;
}

class MockDirectoryViewContainer : public DirectoryViewContainer
{
public:
    // add the constructor and pass the parent to the base class constructor.
    explicit MockDirectoryViewContainer(QWidget* parent = nullptr) : DirectoryViewContainer(parent) {}
};

class BackPreDirTest : public Test
{
protected:
    void SetUp() override
    {
        // get the current user's home directory
        QString homeDir = QDir::homePath();

        testDir = QDir(homeDir + "/_xxx_test");
        if (testDir.exists()) {
            if (!testDir.removeRecursively()) {
                qDebug() << "Failed to remove directory: " << testDir.path();
                return;
            }
        }

        // create the “_xxx_test” directory
        if (!testDir.mkdir(homeDir + "/_xxx_test")) {
            qDebug() << "Failed to create directory: " << testDir.path();
            return;
        }

        // create 100 directories under the “_xxx_test” directory.
        for (int i = 1; i <= 100; i++) {
            QString dirName = QString("dir_%1").arg(i, 3, 10, QChar('0')); // naming directories with 3 digits
            QDir dir(testDir.path() + "/" + dirName);
            if (!dir.exists()) {
                if (!dir.mkdir(dir.path())) {
                    qDebug() << "Failed to create directory: " << dir.path();
                    return;
                } else {
                    //qDebug() << "Created directory: " << dir.path();
                }
            }

           if (i == 1)
                oneDir = dir.absolutePath();
            else if (i == 90)
                twoDir = dir.absolutePath();
        }

        parent = new QWidget;
        mock_container = new MockDirectoryViewContainer(parent);
    }

    void TearDown() override {
        // delete the “_xxx_test” directory at the end of a test case
        if (!testDir.removeRecursively()) {
            FAIL() << "Failed to remove directory: " << testDir.path();
        }

        delete mock_container;
    }


protected:
    QDir testDir;
    QString oneDir;
    QString twoDir;
    QWidget* parent = nullptr;
    MockDirectoryViewContainer* mock_container = nullptr;
};

TEST_F(BackPreDirTest, init) {
    // calculate the total number of folders created
    auto dirCount = testDir.entryList(QDir::NoDotAndDotDot | QDir::Dirs).count();
    ASSERT_EQ(dirCount, 100) << "100 folders were not created during the initialization phase";
}

TEST_F(BackPreDirTest, GoBack_CannotGoBack) {
    // testcase 1: goBack() doesn't do anything when it can't go back
    //EXPECT_CALL(*mock_container, updateWindowLocationRequest(_, _)).Times(0);
    mock_container->goBack();
    ASSERT_FALSE(mock_container->canGoBack());
    ASSERT_FALSE(mock_container->canGoForward());
}

TEST_F(BackPreDirTest, GoBack_CanGoBack) {
    // testcase 2: goBack() correctly updates history, sets properties, and sends signals when it can go back
    // access ~/_xxx_test/dir_001
    mock_container->goToUri(oneDir, true);
    ASSERT_FALSE(mock_container->canGoForward());

    // access ~/_xxx_test/dir_090
    mock_container->goToUri(twoDir, true);

    ASSERT_EQ(mock_container->getCurrentUri(), twoDir);
    ASSERT_TRUE(mock_container->canGoBack());

    // create a QSignalSpy to listen for the updateWindowLocationRequest signal.
    QSignalSpy spy(mock_container, SIGNAL(updateWindowLocationRequest(const QString&, bool, bool)));

    qDebug() << "Before getBackList(): " << mock_container->getBackList();
    qDebug() << "Before getForwardList(): " << mock_container->getForwardList();

    mock_container->goBack();

    qDebug() << "After getBackList(): " << mock_container->getBackList();
    qDebug() << "After getForwardList(): " << mock_container->getForwardList();

    ASSERT_TRUE(mock_container->canGoForward());
    ASSERT_FALSE(mock_container->canGoBack());
    ASSERT_EQ(mock_container->property("mSelectPreviousFolder").toBool(), true);

    // assertion signal is triggered
    ASSERT_EQ(spy.count(), 1);
    // check signal parameters
    qDebug() << "oneDir:" << oneDir;
    qDebug() << "Signal arguments:" << spy.at(0);
    QList<QVariant> arguments = spy.takeFirst();
    ASSERT_EQ(arguments.at(0).toString(), "");
    ASSERT_FALSE(arguments.at(1).toBool());

}

