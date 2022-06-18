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

#ifndef METADATAEMBLEMPROVIDER_H
#define METADATAEMBLEMPROVIDER_H

#include <QObject>
#include "peony-core_global.h"
#include "emblem-provider.h"

namespace Peony {

class PEONYCORESHARED_EXPORT MetadataEmblemProvider : public EmblemProvider
{
    Q_OBJECT
public:
    static MetadataEmblemProvider *getInstance();

    const QString emblemKey() override;

    QStringList getFileEmblemIcons(const QString &uri) override;

private:
    explicit MetadataEmblemProvider(QObject *parent = nullptr);
};

}

#endif // METADATAEMBLEMPROVIDER_H
