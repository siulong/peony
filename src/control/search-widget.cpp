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

#include "search-widget.h"
#include "advanced-location-bar.h"
#include "search-vfs-uri-parser.h"
#include <QToolButton>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QHBoxLayout>
#include <QDebug>
#include <QStandardPaths>
#include <QParallelAnimationGroup>

using namespace Peony;
static const int SEARCH_BUTTON_SIZE =36;

SearchWidget::SearchWidget(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    m_locationBar = new AdvancedLocationBar(this);

    connect(m_locationBar, &AdvancedLocationBar::refreshRequest, this, &SearchWidget::refreshRequest);

    connect(m_locationBar, &AdvancedLocationBar::updateFileTypeFilter, this, &SearchWidget::updateFileTypeFilter);

    connect(m_locationBar, &AdvancedLocationBar::searchRequest, [=](const QString &path, const QString &key){
        //key is null, clean search content, show all files
        bool searchMode = m_locationBar->getSearchMode();
        if (key == "" || key.isNull()) {
            Q_EMIT this->updateSearchRequest(searchMode);
            Q_EMIT this->updateLocationRequest(path, false);
            this->updateSearch(path, key, true);
            if (!searchMode) {
                m_searchGlobal = false;
            }
        } else {
            if (m_searchGlobal) {
                QString homePath = "file://" + QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
                auto targetUri = Peony::SearchVFSUriParser::parseSearchKey(homePath, key, true, false, "", m_searchRecursive);
                targetUri = targetUri.replace("&recursive=0", "&recursive=1");
                Q_EMIT this->updateLocationRequest(targetUri, false);
                this->updateSearch(homePath, key, true);
            } else {
                Q_EMIT this->updateSearchRequest(searchMode);
                auto targetUri = Peony::SearchVFSUriParser::parseSearchKey(path, key, true, false, "", m_searchRecursive);
                targetUri = targetUri.replace("&recursive=1", "&recursive=0");
                Q_EMIT this->updateLocationRequest(targetUri, false);
                this->updateSearch(path, key, true);
            }
        }
    });

    connect(m_locationBar, &AdvancedLocationBar::updateWindowLocationRequest, this, &SearchWidget::updateLocationRequest);
    connect(this, &SearchWidget::updateLocation, m_locationBar, &AdvancedLocationBar::updateLocation);
    connect(this, &SearchWidget::cancelEdit, m_locationBar, &AdvancedLocationBar::cancelEdit);
    connect(this, &SearchWidget::finishEdit, m_locationBar, &AdvancedLocationBar::finishEdit);
    connect(this, &SearchWidget::clearSearchBox, m_locationBar, &AdvancedLocationBar::clearSearchBox);

    layout->addWidget(m_locationBar);
    layout->setSpacing(102);
}

void SearchWidget::searchButtonClicked()
{
    m_searchMode = ! m_searchMode;
    qDebug() << "searchButtonClicked" <<m_searchMode;
    Q_EMIT this->updateSearchRequest(m_searchMode);
    setSearchMode(m_searchMode);
}

void SearchWidget::setSearchMode(bool mode)
{
    m_locationBar->switchEditMode(mode);
}

void SearchWidget::closeSearch()
{
    m_searchMode = false;
    setSearchMode(false);
}

void SearchWidget::startEdit(bool bSearch)
{
    //qDebug() << "bSearch" <<bSearch <<m_searchMode;
    if (bSearch && m_searchMode) {
        m_locationBar->setSearchBarFocus();
        return;
    }

    if (bSearch) {
        searchButtonClicked();
    } else {
        m_locationBar->startEdit();
        m_locationBar->switchEditMode(false);
        if (m_searchMode) {
            Q_EMIT updateSearchRequest(false);
        }
        m_searchMode = false;
    }
}

void SearchWidget::setGlobalFlag(bool isGlobal)
{
    m_searchGlobal = isGlobal;
    m_locationBar->deselectSearchBox();
}

void SearchWidget::updateSearchRecursive(bool recursive)
{
    m_searchRecursive = recursive;
}

bool SearchWidget::isSearchMode()
{
    return m_searchMode;
}

void SearchWidget::updateTabletModeValue(bool isTabletMode)
{
    //task#106007 【文件管理器】文件管理器应用做平板UI适配，去掉搜索
    int height = isTabletMode? 48:36;
    m_locationBar->setFixedHeight(height);
}

void SearchWidget::updateSearchProgress(bool isSearching)
{
    m_locationBar->updateSearchProgress(isSearching);
}
