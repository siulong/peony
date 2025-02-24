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

#include "desktop-item-proxy-model.h"
#include "file-info.h"
#include "file-meta-info.h"

#include "file-operation-utils.h"
#include "global-settings.h"

#include <QDir>
#include <QDebug>

#include <QLocale>
#include <QCollator>
#include <QStandardPaths>
#include <QUrl>

using namespace Peony;

QLocale locale = QLocale(QLocale::system().name());
QCollator comparer = QCollator(locale);

bool startWithChinese(const QString &displayName)
{
    //NOTE: a newly created file might could not get display name soon.
    if (displayName.isEmpty()) {
        return false;
    }
    auto firstStrUnicode = displayName.at(0).unicode();
    return (firstStrUnicode <=0x9FA5 && firstStrUnicode >= 0x4E00);
}

DesktopItemProxyModel::DesktopItemProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    //enable number sort, like 100 is after 99
    comparer.setNumericMode(true);

    setSortCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(false);
    auto settings = GlobalSettings::getInstance();
    m_show_hidden = settings->isExist(SHOW_HIDDEN_PREFERENCE)? settings->getValue(SHOW_HIDDEN_PREFERENCE).toBool(): false;
    connect(GlobalSettings::getInstance(), &GlobalSettings::valueChanged, this, [=] (const QString& key) {
        if (SHOW_HIDDEN_PREFERENCE == key) {
            m_show_hidden= GlobalSettings::getInstance()->getValue(key).toBool();
            invalidateFilter();
            Q_EMIT showHiddenFile();
        }
    });
    //qDebug() <<"DesktopItemProxyModel:" <<settings->isExist(SHOW_HIDDEN_PREFERENCE)<<m_show_hidden;

    m_sort_type = settings->getValue(LAST_DESKTOP_SORT_ORDER).toInt();
    m_sort_order = settings->getValue(DESKTOP_SORT_ORDER).toInt();

    m_bwListInfo = new BWListInfo();
    m_jsonOp = new PeonyJsonOperation();
    QString jsonPath=QDir::homePath()+"/.config/peony-security-config.json";
    m_jsonOp->setConfigFile(jsonPath);
    m_jsonOp->loadConfigFile(m_bwListInfo);
}

DesktopItemProxyModel::~DesktopItemProxyModel()
{
    delete m_jsonOp;
    delete m_bwListInfo;
}

bool DesktopItemProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!sourceModel())
        return false;

    auto sourceIndex = sourceModel()->index(source_row, 0, source_parent);
    if (sourceIndex.data().toString().isEmpty())
        return false;
    auto uri = sourceIndex.data(Qt::UserRole).toString();
    auto info = FileInfo::fromUri(uri);

    //qDebug()<<"fiter"<<uri<<info->displayName();
    if (info->displayName().isNull()) {
        //return false;
    }
    if (! m_show_hidden && info->isHiddenFile()) {
        return false;
    }
    //task#74174 通过id筛选出需要在该view中显示项
    int id = sourceIndex.data(Qt::UserRole+3).toInt();
    QPoint pos = sourceIndex.data(Qt::UserRole + 2).toPoint();

    if (m_id != id) {
        return false;
    }

    //fix desktop show Desktop folder issue, bug#20293
    if (QUrl(uri).path() == QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/Desktop"
        || QUrl(uri).path() == QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Desktop" + "/Desktop")
    {
        if (! QFile::exists(QUrl(uri).path()))
           return false;
    }

//    if (!GlobalSettings::getInstance()->getValue(DISPLAY_STANDARD_ICONS).toBool()) {
//        if (uri == "computer:///"
//            || uri == "trash:///"
//            || uri == ("file://" + QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
//            || uri == QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
//        {
//            return false;
//        }
//    }

    //家目录图标显示隐藏，只要DISPLAY_STANDARD_ICONS 或者 HOME_ICON_VISIBLE 其中任意1个为false值即可
    //但是如果改为隐藏后，需要显示，必须保证两个值都为true, 回收站和计算机图标为同样逻辑
    bool display_standard_icons = GlobalSettings::getInstance()->getValue(DISPLAY_STANDARD_ICONS).toBool();
    if (! GlobalSettings::getInstance()->getValue(HOME_ICON_VISIBLE).toBool() || ! display_standard_icons) {
        if (uri == ("file://" + QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
            || uri == QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
        {
            return false;
        }
    }

    if (((! GlobalSettings::getInstance()->getValue(TRASH_ICON_VISIBLE).toBool() || ! display_standard_icons)
         && uri == "trash:///")) {
       return false;
    }

    if (((! GlobalSettings::getInstance()->getValue(COMPUTER_ICON_VISIBLE).toBool() || ! display_standard_icons)
         && uri == "computer:///")) {
       return false;
    }

    if (info->isDesktopFile() && nullptr != info->desktopName()){
        qDebug() << "desktopName"<<info->desktopName();
        if (m_bwListInfo->isBlackListMode()){
            return !m_bwListInfo->desktopNameExist(info->desktopName());
        } else if (m_bwListInfo->isWriteListMode()){
            return m_bwListInfo->desktopNameExist(info->desktopName());
        }
    }

    return true;
}

bool DesktopItemProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    //qDebug()<<"less than";
    if (m_sort_type == Other) {
        return true;
    }

    auto leftUri = source_left.data(Qt::UserRole).toString();
    auto leftInfo = FileInfo::fromUri(leftUri);
    //auto leftMetaInfo = FileMetaInfo::fromUri(leftUri);

    auto rightUri = source_right.data(Qt::UserRole).toString();
    auto rightInfo = FileInfo::fromUri(rightUri);
    //auto rightMetaInfo = FileMetaInfo::fromUri(rightUri);

    //computer home and trash first
    if (source_left.row() < 3) {
        if (source_right.row() < source_left.row()) {
            return (sortOrder()==Qt::AscendingOrder)? false: true;
        }
        return (sortOrder()==Qt::AscendingOrder)? true: false;
    }
    if (source_right.row() < 3) {
        if (source_left.row() < source_right.row()) {
            return (sortOrder()==Qt::AscendingOrder)? true: false;
        }
        return (sortOrder()==Qt::AscendingOrder)? false: true;
    }

    //dir first
    if (leftInfo->isDir()) {
        if (rightInfo->isDir()) {
            //fix bug#89115, folders not sort by name
            if (m_sort_type != ModifiedDate){
                if (leftInfo->isDir() && rightInfo->isDir())
                {
                    goto default_sort;
                }
            }
        } else {
            return (sortOrder()==Qt::AscendingOrder)? true: false;
        }
    } else {
        if (rightInfo->isDir()) {
            return (sortOrder()==Qt::AscendingOrder)? false: true;
        }
    }

    //qDebug()<<"sort in desktop"<<SortType(m_sort_type)<<m_sort_type;
    switch (m_sort_type) {
    case FileName: {
        if (FileOperationUtils::leftNameIsDuplicatedFileOfRightName(leftInfo->displayName(), rightInfo->displayName())) {
            return FileOperationUtils::leftNameLesserThanRightName(leftInfo->displayName(), rightInfo->displayName());
        }
        goto default_sort;
    }
    case ModifiedDate: {
        if (leftInfo->modifiedTime() == rightInfo->modifiedTime())
            goto default_sort;
        return leftInfo->modifiedTime() > rightInfo->modifiedTime();
    }
    case FileType: {
        if (leftInfo->fileType() == rightInfo->fileType())
            goto default_sort;

        QString leftDisplayType = leftInfo->fileType();
        QString rightDisplayType = rightInfo->fileType();

        if (startWithChinese(leftDisplayType)) {
            if (!startWithChinese(rightDisplayType)) {
                return (sortOrder()==Qt::AscendingOrder)? true: false;
            } else {
                //chinese pinyin sort order is reversed compared with english.
                //return !QSortFilterProxyModel::lessThan(source_left, source_right);
                //fix bug#89115, chinese files not sort by name pinyin
                return comparer.compare(leftDisplayType, rightDisplayType) > 0;
            }
        } else {
            if (startWithChinese(rightDisplayType)) {
                return (sortOrder()==Qt::AscendingOrder)? false: true;
            }
            return comparer.compare(leftDisplayType, rightDisplayType) > 0;
        }
    }
    case FileSize: {
        if (leftInfo->size() == rightInfo->size())
            goto default_sort;
        return leftInfo->size() > rightInfo->size();
    }
    }

default_sort:
    //when sort value is same, use name to sort, fix refresh change order issue
    //fix bug#99928, desktop sort not same with folder issue, and releated to bug#92525
    QString leftDisplayName = leftInfo->displayName();
    QString rightDisplayName = rightInfo->displayName();

    if (startWithChinese(leftInfo->displayName())) {
        if (!startWithChinese(rightInfo->displayName())) {
            return (sortOrder()==Qt::AscendingOrder)? true: false;
        } else {
            //chinese pinyin sort order is reversed compared with english.
            //return !QSortFilterProxyModel::lessThan(source_left, source_right);
            //fix bug#89115, chinese files not sort by name pinyin
            return comparer.compare(leftInfo->displayName(), rightInfo->displayName()) > 0;
        }
    } else {
        if (startWithChinese(rightInfo->displayName())) {
            return (sortOrder()==Qt::AscendingOrder)? false: true;
        }
        return comparer.compare(leftDisplayName, rightDisplayName) > 0;
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

void DesktopItemProxyModel::setShowHidden(bool showHidden)
{
    GlobalSettings::getInstance()->setGSettingValue(SHOW_HIDDEN_PREFERENCE, showHidden);
    m_show_hidden = showHidden;
    invalidateFilter();
}

int DesktopItemProxyModel::updateBlackAndWriteLists()
{
    m_bwListInfo->clearBWlist();
    m_jsonOp->loadConfigFile(m_bwListInfo);
    //重新过滤显示
    invalidateFilter();
    return 0;
}

void DesktopItemProxyModel::invalidateModel()
{
    invalidateFilter();
}

void DesktopItemProxyModel::setId(int id)
{
    m_id = id;
}

QString DesktopItemProxyModel::getBlackAndWhiteModel()
{
    return m_bwListInfo->getMode();
}

bool DesktopItemProxyModel::getBlackAndWhiteListExist(QString name)
{
    return m_bwListInfo->desktopNameExist(name);
}

QSet<QString> DesktopItemProxyModel::getBWListInfo()
{
    return m_bwListInfo->getBWListInfo();
}

void DesktopItemProxyModel::setDesktopUseAutoLayout(bool desktopUseAutoLayout)
{
    m_desktopUseAutoLayout = desktopUseAutoLayout;
}

bool DesktopItemProxyModel::getDesktopUseAutoLayout() const
{
    return m_desktopUseAutoLayout;
}

QStringList DesktopItemProxyModel::getSortedUris() const
{
    return m_sortedUris;
}

void DesktopItemProxyModel::setSortedUris(const QStringList &sortedUris)
{
    m_sortedUris = sortedUris;
//    Q_EMIT requestSyncSortedUrisWithDelay();
}

QModelIndexList DesktopItemProxyModel::getAllFileIndexes()
{
    QModelIndexList l;
    int i = 0;
    while (this->index(i, 0, QModelIndex()).isValid()) {
        auto index = this->index(i, 0, QModelIndex());
        if (m_show_hidden) {
            l<<index;
        } else {
            auto disyplayName = index.data(Qt::DisplayRole).toString();
            if (disyplayName.isEmpty()) {
                auto uri = this->index(i, 0, QModelIndex()).data(Qt::UserRole).toString();
                disyplayName = FileUtils::getFileDisplayName(uri);
            }
            if (!disyplayName.startsWith(".")) {
                l<<index;
            }
        }

        i++;
    }
    return l;
}
