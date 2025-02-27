/*
 * Peony-Qt
 *
 * Copyright (C) 2020, KylinSoft Co., Ltd.
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

#ifndef DESKTOPITEMPROXYMODEL_H
#define DESKTOPITEMPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "bw-list-info.h"
#include "peony-json-operation.h"

namespace Peony {

class DesktopItemProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum SortType {
        FileName,
        ModifiedDate,
        FileType,
        FileSize,
        Other
    };
    explicit DesktopItemProxyModel(QObject *parent = nullptr);
    ~DesktopItemProxyModel();

    void setSortType(int type) {
        m_sort_type = type;
    }
    int getSortType() {
        return m_sort_type;
    }

    void setSortOrder(int sortOrder) {
        m_sort_order = sortOrder;
    }

    int getSortOrder() {
        return m_sort_order;
    }

    void setShowHidden(bool showHidden);
    int updateBlackAndWriteLists();
    QString getBlackAndWhiteModel();
    QSet<QString> getBWListInfo();
    bool getBlackAndWhiteListExist(QString name);


    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;

    void invalidateModel();
    void setId(int id);

    QModelIndexList getAllFileIndexes();

    void setDesktopUseAutoLayout(bool desktopUseAutoLayout);
    bool getDesktopUseAutoLayout() const;

    QStringList getSortedUris() const;
    void setSortedUris(const QStringList &sortedUris);

Q_SIGNALS:
    void showHiddenFile();
    void requestSyncSortedUrisWithDelay(int msec = 1000);

private:
    int m_sort_type = Other;
    bool m_show_hidden;
    int m_sort_order = 0;
    BWListInfo    *m_bwListInfo;
    PeonyJsonOperation *m_jsonOp;
    int m_id = 0;

    bool m_desktopUseAutoLayout = false;
    QStringList m_sortedUris;
};

}

#endif // DESKTOPITEMPROXYMODEL_H
