/*
 * Peony-Qt's Library
 *
 * Copyright (C) 2024, KylinSoft Co., Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Yue Lan <lanyue@kylinos.cn>
 *
 */

#include "file-properties-operation.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <QFile>
#include <QTextStream>
#include <QString>

using namespace Peony;

void appendToFileLineByLine(const QString &filePath, const QString &content) {
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << content << endl; // 追加内容并添加换行符
    }
    file.close();
}

void deleteLineContainingTextLineByLine(const QString &filePath, const QString &textToDelete) {
    QFile originalFile(filePath);
    QString tempFilePath = filePath + ".tmp";
    QFile tempFile(tempFilePath);

    if (originalFile.open(QIODevice::ReadOnly | QIODevice::Text) && tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream in(&originalFile);
        QTextStream out(&tempFile);
        QString line;
        while (!in.atEnd()) {
            line = in.readLine();
            if (!line.contains(textToDelete)) {
                out << line << endl; // 写入不包含特定文本的行
            }
        }
    }

    originalFile.close();
    tempFile.close();

    // 删除原文件并将临时文件重命名为原文件
    originalFile.remove();
    tempFile.rename(filePath);
}

FilePropertiesOperation::FilePropertiesOperation(const QStringList &uris, FilePropertiesOperation::Options options, bool setHidden, bool setReadOnly, QObject *parent)
{
    m_uris = uris;
    m_shouldSetHidden = setHidden;
    m_shouldSetReadOnly = setReadOnly;
    m_options = options;
    m_reporter = new FileNodeReporter(this);
    connect(m_reporter, &FileNodeReporter::nodeFound, this, [=](const QString &uri){
        Q_EMIT this->operationPreparedOne(uri, 0);
    });
    m_info = std::make_shared<FileOperationInfo>(uris, uris, FileOperationInfo::Other);
}

void FilePropertiesOperation::run()
{
    Q_EMIT operationStarted();
    if (m_uris.isEmpty())
        Q_EMIT operationFinished();

    QList<FileNode *> nodes;
    if (m_options.testFlag(ChangeRecursively)) {
        for (auto uri : m_uris) {
            auto node = new FileNode(uri, nullptr, m_reporter);
            node->findChildrenRecursively();
            nodes<<node;
        }
        Q_EMIT operationPrepared();

        bool cancelled = isCancelled();
        for (auto node : nodes) {
            setPropertiesRecursively(node, &cancelled);
        }

        Q_EMIT operationFinished();
    } else {
        for (auto uri : m_uris) {
            auto node = new FileNode(uri, nullptr, nullptr);
            nodes<<node;
            Q_EMIT this->operationPreparedOne(uri, 0);
        }

        for (auto node : nodes) {
            if (isCancelled()) {
                break;
            }

            setPropertiesOne(node);
        }
    }

    Q_EMIT operationFinished();

    for (auto node : nodes) {
        delete node;
    }
}

std::shared_ptr<FileOperationInfo> FilePropertiesOperation::getOperationInfo()
{
    return m_info;
}

void FilePropertiesOperation::setPropertiesRecursively(FileNode *node, bool *cancelled)
{
    if (*cancelled) {
        return;
    }

    if (isCancelled()) {
        *cancelled = true;
        return;
    }

    if (node->isFolder()) {
        for (auto childNode : *node->children()) {
            setPropertiesRecursively(childNode, cancelled);
        }
        if (m_options.testFlag(ChangeHidden)) {
            if (m_shouldSetHidden) {
                //将该目录下所有非隐藏文件的名称写入.hidden中
                g_autoptr (GFile) folder = g_file_new_for_uri(node->uri().toUtf8().constData());
                g_autoptr (GFile) hidden_file = g_file_resolve_relative_path(folder, ".hidden");
                QStringList filenameList;
                for (FileNode *childNode : *(node->children())) {
                    g_autoptr (GFile) child_file = g_file_new_for_uri(childNode->uri().toUtf8().constData());
                    g_autofree gchar *child_file_basename = g_file_get_basename(child_file);
                    childNode->setDestFileName(child_file_basename);
                    if (!childNode->destBaseName().startsWith(".")) {
                        filenameList.append(childNode->destBaseName());
                    }
                }
                g_autofree gchar *hidden_file_path = g_file_get_path(hidden_file);
                QString contents = filenameList.join('\n');
                if (hidden_file_path)
                    g_file_set_contents(hidden_file_path, contents.toUtf8().constData(), -1, nullptr);

                //隐藏自身
                //setPropertiesOne(node);
            } else {
                //删除该目录下的.hidden文件
                g_autoptr (GFile) folder = g_file_new_for_uri (node->uri().toUtf8().constData());
                g_autoptr (GFile) hidden_file = g_file_resolve_relative_path(folder, ".hidden");
                g_file_delete (hidden_file, nullptr, nullptr);

                //取消自身隐藏
                //setPropertiesOne(node);
            }
        }
    } else {
        setPropertiesOne(node);
    }
    Q_EMIT this->FileProgressCallback(node->uri(), node->uri(), "", 0, 0);
}

void FilePropertiesOperation::setPropertiesOne(FileNode *node)
{
    g_autoptr (GFile) file = g_file_new_for_uri(node->uri().toUtf8().constData());
    g_autofree gchar *child_file_basename = g_file_get_basename(file);
    node->setDestFileName(child_file_basename);
    node->setDestUri(node->uri());

    GError *err = nullptr;
    bool shouldManuallyChanged = false;
    if (m_options.testFlag(ChangeHidden)) {
        // 对于顶级节点，如果没有修改unix-mode，需要手动触发文件更新以隐藏选项便数据同步
        shouldManuallyChanged = !node->parent();
        if (m_options.testFlag(ChangeReadOnly)) {
            shouldManuallyChanged = false;
        }
        if (m_shouldSetHidden) {
            // FIXME: 兼容原来形式
            // FIXME: 优化性能，顶层节点如果在同一个目录下可以统一处理
            if (!node->parent()) {
                // 递归处理的隐藏设置由上一级目录设置时处理，此处仅处理顶级节点
                g_autoptr (GFile) directory = g_file_get_parent(file);
                g_autoptr (GFile) hidden_file = g_file_resolve_relative_path(directory, ".hidden");
                g_autofree gchar *hidden_file_path = g_file_get_path(hidden_file);
                appendToFileLineByLine(hidden_file_path, node->destBaseName());
            }
        } else {
            // 兼容"."前缀取消隐藏
            if (node->destBaseName().startsWith(".")) {
                auto destName = node->destBaseName();
                destName.remove(0, 1);
                g_autoptr (GFile) new_file = g_file_set_display_name(file, destName.toUtf8().constData(), nullptr, &err);
                if (!err) {
                    g_autofree gchar *new_uri = g_file_get_uri(new_file);
                    node->setDestUri(new_uri);
                } else {
                    // FIXME: 与errorhandler交互
                    qWarning()<<"set file properties op error:"<<node->destBaseName()<<err->message;
                }
            } else {
                // FIXME: 优化性能，顶层节点如果在同一个目录下可以统一处理
                if (!node->parent()) {
                    // 递归处理的隐藏设置由上一级目录设置时处理，此处仅处理顶级节点
                    g_autoptr (GFile) directory = g_file_get_parent(file);
                    g_autoptr (GFile) hidden_file = g_file_resolve_relative_path(directory, ".hidden");
                    g_autofree gchar *hidden_file_path = g_file_get_path(hidden_file);
                    deleteLineContainingTextLineByLine(hidden_file_path, node->destBaseName());
                }
            }
        }
    }

    if (m_options.testFlag(ChangeReadOnly) && !node->isFolder()) {
        g_autoptr (GFile) file = g_file_new_for_uri(node->destUri().toUtf8().constData());
        g_autofree gchar *path = g_file_get_path(file);
        guint32 unixMode = 0755;
        struct stat file_stat;
        int ret = stat(path, &file_stat);
        if (ret == -1) {
            qCritical()<<"failed to get file permission"<<path;
        } else {
            unixMode = file_stat.st_mode;
        }

        if (m_shouldSetReadOnly) {
            unixMode &= 0555;
        } else {
            unixMode |= 0222;
        }
        if (path) {
            g_chmod(path, unixMode);
        }
    }

    if (shouldManuallyChanged) {
        //更新文件
        g_autoptr (GFile) new_file = g_file_new_for_uri(node->destUri().toUtf8().constData());
        g_autoptr (GFileInfo) info = g_file_query_info(new_file, G_FILE_ATTRIBUTE_TIME_ACCESS, G_FILE_QUERY_INFO_NONE, nullptr, nullptr);
        g_file_set_attribute_uint64(new_file, G_FILE_ATTRIBUTE_TIME_ACCESS, g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_ACCESS), G_FILE_QUERY_INFO_NONE, nullptr, nullptr);
    }
}
