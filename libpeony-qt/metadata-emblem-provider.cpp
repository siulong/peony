/*
 * Peony-Qt
 *
 * Copyright (C) 2023, KylinSoft Information Technology Co., Ltd.
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
 *
 */


#include "metadata-emblem-provider.h"
#include "file-meta-info.h"

#include <QApplication>
#include <QThread>

using namespace Peony;

static MetadataEmblemProvider *global_instance = nullptr;

MetadataEmblemProvider *MetadataEmblemProvider::getInstance()
{
    if (!global_instance)
        global_instance = new MetadataEmblemProvider;
    return global_instance;
}

const QString MetadataEmblemProvider::emblemKey()
{
    return "metainfo-emblem";
}

QStringList MetadataEmblemProvider::getFileEmblemIcons(const QString &uri)
{
    std::shared_ptr<Peony::FileMetaInfo> metaInfo = nullptr;
    if (QThread::currentThread() == qApp->thread()) {
        metaInfo = FileMetaInfo::fromUri(uri);
    } else {
        metaInfo = requestDupMetaInfo(uri);
    }
    if(!metaInfo || !metaInfo.get())
        return QStringList();
    return metaInfo->getMetaInfoStringListV1("emblems");
}

MetadataEmblemProvider::MetadataEmblemProvider(QObject *parent) : EmblemProvider(parent)
{
    connect(this, &MetadataEmblemProvider::requestDupMetaInfo, this, &MetadataEmblemProvider::getDupMetaInfo, Qt::BlockingQueuedConnection);
}

std::shared_ptr<FileMetaInfo> MetadataEmblemProvider::getDupMetaInfo(const QString &uri)
{
    return FileMetaInfo::dupFromUri(uri);
}
