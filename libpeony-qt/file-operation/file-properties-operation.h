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

#ifndef FILEPROPERTIESOPERATION_H
#define FILEPROPERTIESOPERATION_H

#include <QObject>

#include "peony-core_global.h"

#include "file-operation.h"

#include "file-node-reporter.h"
#include "file-node.h"

namespace Peony {

class PEONYCORESHARED_EXPORT FilePropertiesOperation : public FileOperation
{
    Q_OBJECT
public:
    enum Mode {
        NoOp = 0,
        ChangeHidden = 1 << 0,
        ChangeReadOnly = 1 << 1,
        ChangeRecursively = 1 << 2
    };
    Q_ENUM (Mode)
    Q_DECLARE_FLAGS (Options, Mode)

    explicit FilePropertiesOperation(const QStringList &uris, Options mode = NoOp, bool setHidden = false, bool setReadOnly = false, QObject *parent = nullptr);

    void run() override;

    std::shared_ptr<FileOperationInfo> getOperationInfo();

protected:
    void setPropertiesRecursively(FileNode *node, bool *cancelled);
    void setPropertiesOne(FileNode *node);

private:
    QStringList m_uris;
    bool m_shouldSetHidden = false;
    bool m_shouldSetReadOnly = false;
    Options m_options;

    FileNodeReporter *m_reporter = nullptr;

    std::shared_ptr<FileOperationInfo> m_info;
};

}

#endif // FILEPROPERTIESOPERATION_H
