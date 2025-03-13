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
#include <gtest/gtest.h>
#include <qtest.h>
#include <QDebug>
#include "stubext.h"
#include "file-operation/create-template-operation.h"
#include "file-operation/file-copy-operation.h"
#include "file-operation/file-move-operation.h"
#include "file-operation/file-rename-operation.h"
#include "file-operation/file-trash-operation.h"
#include "file-operation/file-delete-operation.h"
#include "file-operation/file-link-operation.h"
#include "file-operation/file-operation-error-dialog.h"
#include "file-operation/file-operation-error-dialogs.h"
#include "file-utils.h"
using namespace testing;
//文件基础操作通过性测试，用例验证基础功能能否正常运行
class operationTest : public testing::Test
{
public:
    Peony::FileOperationManager *operationManager = Peony::FileOperationManager::getInstance();
    Peony::CreateTemplateOperation *createOp = nullptr;
    Peony::FileCopyOperation* copyOp = nullptr;
    Peony::FileMoveOperation* moveOp = nullptr;
    Peony::FileRenameOperation* renameOp = nullptr;
    Peony::FileTrashOperation* trashOp = nullptr;
    Peony::FileDeleteOperation* deleteOp = nullptr;
    Peony::FileLinkOperation* linkOp = nullptr;
    Peony::FileOperationErrorDialog* dialogTest = nullptr;
private:
    //打桩声明
    stub_ext::StubExt stub;
    QString destDirUris = "file:///data/test/";

};

TEST_F(operationTest, createFile)
{
    createOp = new Peony::CreateTemplateOperation(destDirUris, Peony::CreateTemplateOperation::EmptyFile, "test");
    createOp->run();
    bool isFileExsit = Peony::FileUtils::isFileExsit(destDirUris + "/NewFile.txt");
    ASSERT_TRUE(isFileExsit);
}

TEST_F(operationTest, copyFile)
{
    QStringList sourceUris;
    sourceUris << "file:///data/test/NewFile.txt";
    destDirUris = "file:///data";
    copyOp = new Peony::FileCopyOperation(sourceUris, destDirUris);
    copyOp->run();
    bool isFileExsit = Peony::FileUtils::isFileExsit(destDirUris + "/NewFile.txt");
    ASSERT_TRUE(isFileExsit);
}

TEST_F(operationTest, DeleteFile)
{
    QStringList sourceUris;
    destDirUris = "file:///data/test2";
    createOp = new Peony::CreateTemplateOperation(destDirUris, Peony::CreateTemplateOperation::EmptyFile, "test");
    createOp->run();
    sourceUris << "file:///data/test2/NewFile.txt";
    deleteOp = new Peony::FileDeleteOperation(sourceUris);
    operationManager->startOperation(deleteOp, true);
    bool isFileExsit = Peony::FileUtils::isFileExsit(sourceUris.first() + "/NewFile.txt");
    ASSERT_FALSE(isFileExsit);
}

TEST_F(operationTest, RenameFile)
{
    destDirUris = "file:///data/test/NewFile.txt";
    renameOp = new Peony::FileRenameOperation(destDirUris,"abc.txt");
    renameOp->run();
    bool isFileExsit = Peony::FileUtils::isFileExsit(QString("file:///data/test") + "/abc.txt");
    ASSERT_TRUE(isFileExsit);
}


TEST_F(operationTest, MoveFile)
{
    QStringList sourceUris;
    sourceUris << "file:///data/test/abc.txt";
    destDirUris = "file:///data";
    moveOp = new Peony::FileMoveOperation(sourceUris, destDirUris);
    moveOp->run();
    bool isFileExsit = Peony::FileUtils::isFileExsit(destDirUris + "/abc.txt");
    ASSERT_TRUE(isFileExsit);
}

TEST_F(operationTest, TrashFile)
{
    QStringList sourceUris;
    sourceUris << "file:///data/abc.txt";
    trashOp = new Peony::FileTrashOperation(sourceUris);
    trashOp->run();
    bool isFileExsit = Peony::FileUtils::isFileExsit(destDirUris + "/abc.txt");
    ASSERT_FALSE(isFileExsit);
}

TEST_F(operationTest, dialogTest)
{
    dialogTest = new Peony::FileOperationErrorDialog();
}

TEST_F(operationTest, linkTest)
{
    QString srcUri = "file:///data/NewFile.txt";
    QString desktopUri = "file:///data/test";
    linkOp = new Peony::FileLinkOperation(srcUri, desktopUri);
    linkOp->run();
}

