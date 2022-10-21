/*
 * Peony-Qt
 *
 * Copyright (C) 2022, KylinSoft Co., Ltd.
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
    auto metaInfo = FileMetaInfo::dupFromUri(uri);
    if(!metaInfo || !metaInfo.get())
        return QStringList();
    return metaInfo->getMetaInfoStringListV1("emblems");
}

MetadataEmblemProvider::MetadataEmblemProvider(QObject *parent) : EmblemProvider(parent)
{

}
