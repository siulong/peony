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
 * Authors: yangyanwei <yangyanwei@kylinos.cn>
 *
 */

#include "advanced-desktop-item-model.h"

#include "file-enumerator.h"
#include "file-info.h"
#include "file-info-job.h"
#include "file-info-manager.h"
#include "file-watcher.h"
#include "file-operation-manager.h"
#include "file-move-operation.h"
#include "file-trash-operation.h"
#include "file-copy-operation.h"
#include "file-operation-utils.h"

#include "thumbnail-manager.h"
#include "usershare-manager.h"

#include "file-meta-info.h"

#include "peony-desktop-application.h"
#include "advanced-desktop-icon-view.h"
#include "global-settings.h"
#include "sound-effect.h"
#ifdef KY_SDK_SOUND_EFFECTS
#include "ksoundeffects.h"
#endif
#include "desktop-icon-view-delegate.h"
#include "desktop-menu-plugin-manager.h"
#include "emblem-provider.h"
#include "desktop-window-manager.h"
#include "tooltips-manager.h"

#include <QStandardPaths>
#include <QIcon>

#include <QMimeData>
#include <QUrl>

#include <QTimer>

#include <QMessageBox>

#include <QDebug>

#include "common.h"
using namespace Peony;
#ifdef KY_SDK_SOUND_EFFECTS
using namespace kdk;
#endif

static bool initMetaInfo = false;

AdvancedDesktopIconItem::AdvancedDesktopIconItem(const QString &text)
    : QStandardItem(text)
{
    setData(text, AdvancedDesktopIconView::UriRole);
    auto metaInfo = FileMetaInfo::fromUri(text);
    if (metaInfo) {
        QStringList strPos = metaInfo->getMetaInfoStringListV1(ITEM_GRID_POS_ATTRIBUTE);
        qDebug() << "[AdvancedDesktopIconItem::AdvancedDesktopIconItem]"<<strPos <<text;
        if (2 == strPos.count() && strPos[0] >= 0 && strPos[1] >= 0) {
            QPoint pos(strPos[0].toInt(), strPos[1].toInt());
            setData(pos, AdvancedDesktopIconView::PositionRole);
            int id = metaInfo->getMetaInfoInt(SCREEN_ID);
            setData(id, AdvancedDesktopIconView::ScreenIdRole);
        } else {
            int id;
            QPoint currentPos;
            PeonyDesktopApplication::getDesktopWindowManager()->createIdAndPos(&id, &currentPos);
            setData(id, AdvancedDesktopIconView::ScreenIdRole);
            setData(currentPos, AdvancedDesktopIconView::PositionRole);
        }
        QStringList value = metaInfo->getMetaInfoStringList(RESTORE_ITEM_GRID_POS_ATTRIBUTE);
        setData(value, AdvancedDesktopIconView::ExceptedPositionRole);
        value = metaInfo->getMetaInfoStringList(RESTORE_EXTEND_ITEM_GRID_POS_ATTRIBUTE);
        setData(value, AdvancedDesktopIconView::ExtendScreenPositionRole);
        value = metaInfo->getMetaInfoStringList(RESTORE_SINGLESCREEN_ITEM_GRID_POS_ATTRIBUTE);
        setData(value, AdvancedDesktopIconView::SinglesSreenPositionRole);
    }
}


AdvancedDesktopIconItem::AdvancedDesktopIconItem(const QString &text, bool fileCreate)
    : QStandardItem(text)
{
    setData(text, AdvancedDesktopIconView::UriRole);
    auto metaInfo = FileMetaInfo::fromUri(text);
    if (metaInfo) {
        qDebug() << "[AdvancedDesktopIconItem::AdvancedDesktopIconItem]"<<text;
        QStringList strPos = metaInfo->getMetaInfoStringListV1(ITEM_GRID_POS_ATTRIBUTE);
        int currentId = metaInfo->getMetaInfoInt(SCREEN_ID);
        auto manager = PeonyDesktopApplication::getDesktopWindowManager();
        bool isRenaming = manager->getIconView(currentId)->isRenaming();

        qDebug() << "[AdvancedDesktopIconItem::AdvancedDesktopIconItem]"<<strPos <<text;
        if (isRenaming && 2 == strPos.count() && strPos[0] >= 0 && strPos[1] >= 0) {
            manager->getIconView(currentId)->setRenaming(false);
            QPoint pos(strPos[0].toInt(), strPos[1].toInt());
            setData(pos, AdvancedDesktopIconView::PositionRole);
            setData(currentId, AdvancedDesktopIconView::ScreenIdRole);
            QStringList value = metaInfo->getMetaInfoStringList(RESTORE_ITEM_GRID_POS_ATTRIBUTE);
            setData(value, AdvancedDesktopIconView::ExceptedPositionRole);
            value = metaInfo->getMetaInfoStringList(RESTORE_EXTEND_ITEM_GRID_POS_ATTRIBUTE);
            setData(value, AdvancedDesktopIconView::ExtendScreenPositionRole);
            value = metaInfo->getMetaInfoStringList(RESTORE_SINGLESCREEN_ITEM_GRID_POS_ATTRIBUTE);
            setData(value, AdvancedDesktopIconView::SinglesSreenPositionRole);
        } else {
            int id;
            QPoint currentPos;
            manager->createIdAndPos(&id, &currentPos);
            setData(id, AdvancedDesktopIconView::ScreenIdRole);
            setData(currentPos, AdvancedDesktopIconView::PositionRole);
            QStringList value("");
            setData(value, AdvancedDesktopIconView::ExceptedPositionRole);
            setData(value, AdvancedDesktopIconView::ExtendScreenPositionRole);
            setData(value, AdvancedDesktopIconView::SinglesSreenPositionRole);
        }
    }
}

void AdvancedDesktopIconItem::setData(const QVariant &value, int role)
{
    switch(role){
    case AdvancedDesktopIconView::ScreenIdRole:{
        QString uri = data(AdvancedDesktopIconView::UriRole).toString();
        auto metaInfo = FileMetaInfo::fromUri(uri);
        if (metaInfo) {
            int id = value.toInt();
            metaInfo->setMetaInfoInt(SCREEN_ID, id);
        }
        break;
    }
    case AdvancedDesktopIconView::PositionRole:{
        QString uri = data(AdvancedDesktopIconView::UriRole).toString();
        auto metaInfo = FileMetaInfo::fromUri(uri);
        if (metaInfo) {
            QPoint pos = value.toPoint();
            QStringList itemPos;
            itemPos<<QString::number(pos.x());
            itemPos<<QString::number(pos.y());
            metaInfo->setMetaInfoStringListV1(ITEM_GRID_POS_ATTRIBUTE, itemPos);
        }
        break;
    }
    case AdvancedDesktopIconView::ExceptedPositionRole:{
        QString uri = data(AdvancedDesktopIconView::UriRole).toString();
        auto metaInfo = FileMetaInfo::fromUri(uri);
        if (metaInfo) {
            QStringList restorePos = value.toStringList();
            metaInfo->setMetaInfoStringList(RESTORE_ITEM_GRID_POS_ATTRIBUTE, restorePos);
        }
        break;
    }
    case AdvancedDesktopIconView::SinglesSreenPositionRole:{
        QString uri = data(AdvancedDesktopIconView::UriRole).toString();
        auto metaInfo = FileMetaInfo::fromUri(uri);
        if (metaInfo) {
            QStringList singlesScreenPos = value.toStringList();
            metaInfo->setMetaInfoStringList(RESTORE_SINGLESCREEN_ITEM_GRID_POS_ATTRIBUTE, singlesScreenPos);
        }
        break;
    }
    case AdvancedDesktopIconView::ExtendScreenPositionRole:{
        QString uri = data(AdvancedDesktopIconView::UriRole).toString();
        auto metaInfo = FileMetaInfo::fromUri(uri);
        if (metaInfo) {
            QStringList extendScreenPos = value.toStringList();
            metaInfo->setMetaInfoStringList(RESTORE_EXTEND_ITEM_GRID_POS_ATTRIBUTE, extendScreenPos);
        }
    }
    }
    QStandardItem::setData(value, role);
}

AdvancedDesktopItemModel::AdvancedDesktopItemModel(QObject *parent)
    : QStandardItemModel(parent)
{
    // do not redo layout new items while we start an operation with peony's api.
    connect(FileOperationManager::getInstance(), &FileOperationManager::operationStarted, this, [=](){
        m_items_need_relayout.clear();
    });

    m_thumbnail_watcher = std::make_shared<FileWatcher>("thumbnail:///, this");

    connect(m_thumbnail_watcher.get(), &FileWatcher::fileChanged, this, [=](const QString &uri) {
        auto index = indexFromUri(uri);
        if (index.isValid())
            Q_EMIT this->dataChanged(index, index);
    });

    m_trash_watcher = std::make_shared<FileWatcher>("trash:///", this);

    this->connect(m_trash_watcher.get(), &FileWatcher::fileCreated, [=](const QString &uri) {
        //qDebug()<<"trash changed";
        auto trash = FileInfo::fromUri("trash:///");
        auto job = new FileInfoJob(trash);
        job->setAutoDelete();
        connect(job, &FileInfoJob::infoUpdated, [=]() {
            auto trashIndex = this->indexFromUri("trash:///");
            this->dataChanged(trashIndex, trashIndex);
            Q_EMIT this->requestClearIndexWidget(QStringList()<<uri);
        });
        job->queryAsync();
    });

    this->connect(m_trash_watcher.get(), &FileWatcher::fileDeleted, [=](const QString &uri) {
        //qDebug()<<"trash changed";
        auto trash = FileInfo::fromUri("trash:///");
        auto job = new FileInfoJob(trash);
        job->setAutoDelete();
        connect(job, &FileInfoJob::infoUpdated, [=]() {
            auto trashIndex = this->indexFromUri("trash:///");
            this->dataChanged(trashIndex, trashIndex);
            Q_EMIT this->requestClearIndexWidget(QStringList()<<uri);
        });
        job->queryAsync();
    });

    // monitor desktop
    QString desktopFile = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    qWarning() << "desktopfile:" << desktopFile;
    m_desktop_watcher = std::make_shared<FileWatcher>("file://" + desktopFile, this);
    m_desktop_watcher->setMonitorChildrenChange(true);
    m_desktop_watcher->connect(m_desktop_watcher.get(), &FileWatcher::fileCreated, this, &AdvancedDesktopItemModel::fileCreated);

    m_desktop_watcher->connect(m_desktop_watcher.get(), &FileWatcher::fileDeleted, [=](const QString &uri) {
        for (auto info : m_files) {
            if (info->uri() == uri) {
                int row = m_files.indexOf(info);
                removeRow(row, QModelIndex());
                qDebug() <<"AdvancedDesktopIconView::rowsAboutToBeRemoved" <<row;
                m_files.removeOne(info);
                Q_EMIT sig_relayoutItems();

                if (info->isDir()) {
                    QString displayName = info->displayName();
                    if (UserShareInfoManager::getInstance()->getUsershareLists().contains(displayName)) {
                        SharedDeleteInfoThread *thread = new SharedDeleteInfoThread(info->uri());
                        connect(thread, &SharedDeleteInfoThread::finished, thread, &SharedDeleteInfoThread::deleteLater);
                        thread->start();
                    }
                }
            }
        }

        //fix bug#329662, deleted file not update thumbnail issue
        ThumbnailManager::getInstance()->releaseThumbnail(uri);
    });

    m_desktop_watcher->connect(m_desktop_watcher.get(), &FileWatcher::fileChanged, [=](const QString &uri) {

        for (auto info : m_files) {
            if (info->uri() == uri) {
                auto job = new FileInfoJob(info);
                job->setAutoDelete();
                connect(job, &FileInfoJob::infoUpdated, this, [=]() {
                    //file changed, force create thubnail, link tobug#83108
                    ThumbnailManager::getInstance()->createThumbnail(uri, m_thumbnail_watcher, true);
                    this->dataChanged(indexFromUri(uri), indexFromUri(uri));
                    Q_EMIT this->requestClearIndexWidget(QStringList()<<uri);

                    // task #355895
                    // 触发更新异常软链接，如果一直异常，则会进入轮询知道查询到正常信息
                    if (info->isSymbolLink() && !info->canRead()) {
                        this->pendingQuery(uri);
                    }
                });
                job->queryAsync();
                this->dataChanged(indexFromUri(uri), indexFromUri(uri));
                return;
            }
        }
    });

    //handle standard dir changing.
    m_dir_manager =new UserdirManager(this);
    //refresh after standard dir changed.
    connect(m_dir_manager,&UserdirManager::desktopDirChanged,[=](){
        refresh();
    });
    connect(m_dir_manager,&UserdirManager::thumbnailSetingChange,[=](){
        refresh();
    });

    connect(FileOperationManager::getInstance(), &FileOperationManager::operationStarted, this, [=](std::shared_ptr<FileOperationInfo> info){
        if (info.get()->m_type == FileOperationInfo::Rename) {
            m_renaming_operation_info = info;
            auto renamingUri = info.get()->m_src_uris.first();
            if (!renamingUri.endsWith(".desktop")) {
                m_renaming_operation_info = nullptr;
                return;
            }
            m_renaming_file_pos.first = renamingUri;
            m_renaming_file_pos.second = indexFromUri(renamingUri).data(AdvancedDesktopIconView::PositionRole).toPoint();
        } else {
            m_renaming_file_pos.first = nullptr;
            m_renaming_file_pos.second = QPoint();
            m_renaming_operation_info = nullptr;
        }
    });
    connect(FileOperationManager::getInstance(), &FileOperationManager::operationFinished, this, [=](std::shared_ptr<FileOperationInfo> info){
        if (info.get()->m_type == FileOperationInfo::Rename) {
            if (!info.get()->m_has_error) {
                auto renamingUri = info.get()->target();
                if (!renamingUri.endsWith(".desktop")) {
                    m_renaming_operation_info = nullptr;
                    m_renaming_file_pos.first = nullptr;
                    m_renaming_file_pos.second = QPoint();
                    return;
                }
                QPoint target_pos = indexFromUri(renamingUri).data(AdvancedDesktopIconView::PositionRole).toPoint();
                //desktop文件重命名时，如果存在相同文件则不会重命名成功。由于该文件uri不会变，所以pos不变，无需更新pos
                if (target_pos.isNull() || (m_renaming_file_pos.second == target_pos)) {
                    //desktop文件重命名成功
                    m_renaming_file_pos.first = renamingUri;
                    m_items_need_relayout.removeOne(renamingUri);
                    m_items_need_relayout.removeOne(renamingUri + ".desktop");
                    auto index = indexFromUri(renamingUri);
                    setData(index, m_renaming_file_pos.second, AdvancedDesktopIconView::PositionRole);
                } else {
                    //desktop文件(uri)重命名失败
                    QString &src_uri = info->m_src_uris.first();
                    Q_EMIT selectUri(src_uri);
                }
            } else {
                // restore/relayout?
            }
            m_renaming_operation_info = nullptr;
            QTimer::singleShot(100, this, [=]{
                m_renaming_file_pos.first = nullptr;
                m_renaming_file_pos.second = QPoint();
            });
        } else {
            m_renaming_file_pos.first = nullptr;
            m_renaming_file_pos.second = QPoint();
            m_renaming_operation_info = nullptr;
        }
    });

    auto settings = GlobalSettings::getInstance();
    m_showFileExtension = settings->isExist(SHOW_FILE_EXTENSION)? settings->getValue(SHOW_FILE_EXTENSION).toBool(): true;
    connect(GlobalSettings::getInstance(), &GlobalSettings::valueChanged, this, [=] (const QString& key) {
        if (SHOW_FILE_EXTENSION == key) {
            m_showFileExtension= GlobalSettings::getInstance()->getValue(key).toBool();
            beginResetModel();
            endResetModel();
        }
    });

    connect(DesktopMenuPluginManager::getInstance(), &DesktopMenuPluginManager::pluginLoadFinished, this, [=](){
       QTimer::singleShot(1000, this, [=]{
           for (auto file : m_files) {
               EmblemProviderManager::getInstance()->queryAsync(file->uri());
           }
       });
    });
    UserShareInfoManager::getInstance();

    connect(EmblemProviderManager::getInstance(), &EmblemProviderManager::requestUpdateFile, this, [=](const QString &uri){
        auto index = indexFromUri(uri);
        if (index.isValid())
            Q_EMIT this->dataChanged(index, index);
    });
}

AdvancedDesktopItemModel::~AdvancedDesktopItemModel()
{

}

bool findProgram(const QString &program)
{
    QFileInfo fi(program);
    if (!program.isEmpty() && fi.isExecutable()) {
        return true;
    }

    const QStringList paths = QFile::decodeName(qgetenv("PATH")).split(':');
    for(const QString &dir : paths) {
        QFileInfo fi= QFileInfo(dir + QDir::separator() + program);
        if (fi.isExecutable()) {
            return true;
        }
    }

    return false;
}

void AdvancedDesktopItemModel::refreshInternal()
{
    m_items_need_relayout.clear();
    ThumbnailManager::getInstance()->syncThumbnailPreferences();
    beginResetModel();
    for (auto info : m_files) {
        ThumbnailManager::getInstance()->releaseThumbnail(info->uri());
    }
    m_files.clear();
    m_enumerator = new FileEnumerator(this);
    connect(this, &AdvancedDesktopItemModel::prepareRefresh, m_enumerator, &FileEnumerator::cancel, Qt::DirectConnection);
    m_enumerator->setAutoDelete();
    QString desktopUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    m_enumerator->setEnumerateDirectory(desktopUri);
    m_enumerator->connect(m_enumerator, &FileEnumerator::enumerateFinished, this, &AdvancedDesktopItemModel::onEnumerateFinished);
    m_enumerator->enumerateAsync();
    endResetModel();
}

QVariant AdvancedDesktopItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    //qDebug()<<"data"<<m_files.at(index.row())->uri();
    auto info = m_files.at(index.row());
    switch (role) {
    case Qt::DisplayRole:{
        QString displayName = info->displayName();
        if (info->isDesktopFile())
        {
            displayName = FileUtils::handleDesktopFileName(info->uri(), info->displayName());
            return QVariant(displayName);
        }
        /* story#8359 【文件管理器】手动开启关闭文件拓展名 */
        if(!m_showFileExtension){
            if (info->isDir()) {
                return QVariant(displayName);
            }
            return QVariant(FileUtils::getBaseNameOfFile(displayName));

        }else
            return QVariant(displayName);
    }
    case Qt::ToolTipRole: {
        /**
         * @task #346285: 【Tooltips specification】Change the desktop icon (including the management-desktop-application icon) tooltips display,
         *  add supplementary text
         *
         * @author: Renyg <renyangguang@kylinos.cn>
         * @date:   2024-09-25
         */
        return QVariant(TooltipsManagerInstance.generateTooltip(info.get()));
    }
    case Qt::DecorationRole: {
        if (!info->isExistTargetOfSymlink()) {
            return QIcon::fromTheme("unknown");
        }
        auto thumbnail = ThumbnailManager::getInstance()->tryGetThumbnail(info->uri());
        if (!thumbnail.isNull()) {
            return thumbnail;
        }
        return QIcon::fromTheme(info->iconName(), QIcon::fromTheme("unknown"));
    }
    case UriRole:
        return info->uri();
    case IsLinkRole:
        return info->isSymbolLink();
    default:
        QStandardItem *item = itemFromIndex(index);
        return item ? item->data(role) : QVariant();
    }
    return QVariant();
}

std::shared_ptr<FileInfo> AdvancedDesktopItemModel::getFileInfo(const QModelIndex &index) const
{
    if (!index.isValid() || m_files.isEmpty() || index.row() >= m_files.length())
        return nullptr;

    return m_files.at(index.row());
}

void AdvancedDesktopItemModel::onEnumerateFinished(bool successed)
{
    if (!successed) {
        qWarning()<<"failed to enumerate desktop";
        beginResetModel();
        m_files.clear();
        endResetModel();
        return;
    }

    // check if there is info querying, if true, wait querying finished.
    while (!m_querying_files.isEmpty()) {
        qApp->processEvents();
    }

    if (m_files.count() > 0) {
        beginRemoveRows(QModelIndex(), 0, m_files.count() - 1);
        m_files.clear();
        endRemoveRows();
    }

    auto computer = FileInfo::fromUri("computer:///");
    auto personal = FileInfo::fromPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    auto trash = FileInfo::fromUri("trash:///");

    QList<std::shared_ptr<FileInfo>> infos;

    infos<<computer;
    infos<<trash;
    infos<<personal;

    infos<<m_enumerator->getChildren();
    m_querying_files = infos;
    m_files = infos;

    for (auto info : infos) {
        auto asyncJob = new FileInfoJob(info);
        connect(this, &AdvancedDesktopItemModel::prepareRefresh, asyncJob, [=]{
            asyncJob->cancel();
            asyncJob->setProperty("isCancelled", true);
        }, Qt::DirectConnection);
        connect(asyncJob, &FileInfoJob::queryAsyncFinished, this, [=](bool successed){
            m_querying_files.removeOne(info);
            if (!successed) {
                m_files.removeOne(info);
            }

            if (asyncJob->property("isCancelled").toBool()) {
                // quit loop to avoid invalid data inserted;
                return;
            }

            // task #355895
            // 初始化时无法获取软链接源文件信息，进入轮询
            if (info->isSymbolLink() && !info->canRead()) {
                this->pendingQuery(info->uri());
            }

            if (m_querying_files.isEmpty()) {
                for (auto info : m_files) {
                    auto uri = info->uri();
                    auto item = new AdvancedDesktopIconItem(info->uri());
                    appendRow(item);

                    if (info->isDesktopFile()) {
                        ThumbnailManager::getInstance()->updateDesktopFileThumbnail(info->uri(), m_thumbnail_watcher);
                    } else {
                        ThumbnailManager::getInstance()->createThumbnail(info->uri(), m_thumbnail_watcher);
                    }
                }
                if (!initMetaInfo) {
                    getAllRestoreInfo();
                    initMetaInfo = true;
                    Q_EMIT  emitFinish();
                }
                //qDebug()<<"startMornitor";
                m_trash_watcher->startMonitor();

                qWarning() << "desktopfile:" << m_desktop_watcher->currentUri() << " >>>> " << QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
                if (m_desktop_watcher->currentUri() != "file://" + QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)) {
                    m_desktop_watcher->stopMonitor();
                    m_desktop_watcher->forceChangeMonitorDirectory("file://" + QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
                    m_desktop_watcher->setMonitorChildrenChange(true);
                }
                m_desktop_watcher->startMonitor();

                Q_EMIT refreshed();

                asyncJob->deleteLater();
            }
        });
        asyncJob->queryAsync();
    }
}

const QModelIndex AdvancedDesktopItemModel::indexFromUri(const QString &uri)
{
    for (auto info : m_files) {
        if (info->uri() == uri) {
            return index(m_files.indexOf(info), 0);
        }
    }
    return QModelIndex();
}

QMimeData *AdvancedDesktopItemModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData* data = QAbstractItemModel::mimeData(indexes);
    //set urls data URLs correspond to the MIME type text/uri-list.
    QList<QUrl> urls;
    QStringList uris;
    for (auto index : indexes) {
        QUrl url = index.data(UriRole).toString();
        if (!urls.contains(url))
            urls<<url;
        uris<<index.data(UriRole).toString();
    }
    data->setUrls(urls);
    auto string = uris.join(" ");
    data->setData("peony-qt/encoded-uris", string.toUtf8());
    data->setText(string);
    return data;
}

bool AdvancedDesktopItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    //qDebug()<<row<<column;
    //qDebug()<<"drop mime data"<<parent.data()<<index(row, column, parent).data();
    //judge the drop dest uri.
    QString destDirUri = nullptr;
    if (parent.isValid()) {
        destDirUri = parent.data(UriRole).toString();
    } else {
        destDirUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }

    //if destDirUri was not set, do not execute a drop.
    if (destDirUri.isNull()) {
        return false;
    }

    auto info = FileInfo::fromUri(destDirUri);
    if (info.get()->isEmptyInfo()) {
        // note that this case nearly won't happened.
        // but there is a bug reported due to this.
        // link to task #48798.
        FileInfoJob j(info);
        j.querySync();
    }
    if (!info->isDir()  && ! destDirUri.startsWith("trash:///")) {
        return false;
    }

    //NOTE:
    //do not allow drop on it self.
    auto urls = data->urls();
    if (urls.isEmpty())
        return false;

    bool hasPeonyEncodedUris = false;
    if (data->hasFormat("peony-qt/encoded-uris")) {
        if (!data->data("peony-qt/encoded-uris").isEmpty()) {
            hasPeonyEncodedUris = true;
        }
    }

    QStringList srcUris;
    if (hasPeonyEncodedUris) {
        srcUris = QString(data->data("peony-qt/encoded-uris")).split(" ");
        for (QString uri : srcUris) {
            if (uri.startsWith("recent://"))
                srcUris.removeOne(uri);
        }
    } else {
        for (auto url : urls) {
            //can not drag file from recent
            if (url.url().startsWith("recent://"))
                return false;
            srcUris<<url.url();
        }
    }
    srcUris.removeDuplicates();

    if (srcUris.isEmpty()) {
        return false;
    }

    //can not drag file to recent
    if (destDirUri.startsWith("recent://"))
        return false;

    //not allow drag file to itself
    if (srcUris.contains(destDirUri)) {
        return false;
    }

    //can not move StandardPath to any dir
    if (action == Qt::MoveAction && FileUtils::containsStandardPath(srcUris)) {
        return false;
    }

    bool bMoveFromSearchTab = false;
    if (data->hasFormat("peony-qt/is-search")) {
        bMoveFromSearchTab = QVariant(data->data("peony-qt/is-search")).toBool();
    }

    bool b_trash_item = false;
    for(auto path : srcUris)
    {
        if (path.contains("trash:///"))
        {
            b_trash_item = true;
            break;
        }
    }
    //drag from trash to another place, return false
    //comment to fix can not drag to copy trash file,link to bug#117741
//    if (b_trash_item && destDirUri != "trash:///")
//        return false;

    auto fileOpMgr = FileOperationManager::getInstance();
    bool addHistory = true;
    bool canNotTrash = false;

    for (auto uri : srcUris) {
        if (uri.startsWith("filesafe:///")) {
            canNotTrash = true;
            break;
        }
    }

    if (destDirUri.startsWith("trash:///")) {
        // 如果是保护箱删除时，不会反馈删除弹窗，保护箱文件不影响
        if(!(srcUris.first().startsWith("filesafe:///") &&
            (QString(srcUris.first()).remove("filesafe:///").indexOf("/") == -1))) {
            //fix bug#91525, can trash file in U disk issue
            FileOperationUtils::trash(srcUris, true, bMoveFromSearchTab);
//            if(canNotTrash){
//                FileOperationUtils::trash(srcUris, false);
//            }else {
//                FileTrashOperation *trashOp = new FileTrashOperation(srcUris);
//                fileOpMgr->startOperation(trashOp, addHistory);
//            }
        }
    } else {
        qDebug() << "DesktopItemModel dropMimeData:" <<action;
        //krme files can not move to other place, default set as copy action
        if (srcUris.first().startsWith("kmre:///") || srcUris.first().startsWith("kydroid:///"))
            action = Qt::CopyAction;

        //filesafe files can not move to other place, default set as copy action
        if (srcUris.first().startsWith("filesafe:///"))
            action = Qt::CopyAction;

        //fix drag trash file to other path is copy issue,link to bug#117741
        if (srcUris.first().startsWith("trash:///") && action == Qt::MoveAction){
            //not copy move, do target move to delete file in trash
            action = Qt::TargetMoveAction;
        }

        auto op = FileOperationUtils::moveWithAction(srcUris, destDirUri, true, action, bMoveFromSearchTab);
        op->connect(op, &FileOperation::operationFinished, this, [=](){
            //Peony::SoundEffect::getInstance()->copyOrMoveSucceedMusic();
            //Task#152997, use sdk play sound
            if (op->hasError()) {
                return;
            }
#ifdef KY_SDK_SOUND_EFFECTS
            kdk::KSoundEffects::playSound(SoundType::OPERATION_FILE);
#endif
        }, Qt::BlockingQueuedConnection);
    }

    //NOTE:
    //we have to handle the dnd with file operation, so do not
    //use QAbstractModel::dropMimeData() here;
    return false;
}

void AdvancedDesktopItemModel::refresh()
{
    Q_EMIT prepareRefresh();

    beginResetModel();
    m_files.clear();
    m_items_need_relayout.clear();
    clear();
    endResetModel();

    m_desktop_info = FileInfo::fromPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    auto infoJob = new FileInfoJob(m_desktop_info);
    infoJob->setAutoDelete();
    connect(infoJob, &FileInfoJob::queryAsyncFinished, this, [=](bool successed){
        if (successed) {
            refreshInternal();
        } else {
            qWarning()<<"desktop model refreshs: can not query desktop info"<<m_desktop_info->uri();
        }
    });
    connect(this, &AdvancedDesktopItemModel::prepareRefresh, infoJob, &FileInfoJob::cancel, Qt::DirectConnection);
    infoJob->queryAsync();
}

void AdvancedDesktopItemModel::updateMetaInfo(QStandardItem *item)
{
    //获取当前屏幕的view
    QString uri = item->data(AdvancedDesktopIconView::UriRole).toString();
    auto metaInfo = FileMetaInfo::fromUri(uri);
    if (metaInfo) {
        int id = metaInfo->getMetaInfoInt(SCREEN_ID);
        item->setData(id, AdvancedDesktopIconView::ScreenIdRole);
        QStringList strPos = metaInfo->getMetaInfoStringListV1(ITEM_GRID_POS_ATTRIBUTE);
        if (2 == strPos.count() && strPos[0] >= 0 && strPos[1] >= 0) {
            QPoint pos(strPos[0].toInt(), strPos[1].toInt());
            item->setData(pos, AdvancedDesktopIconView::PositionRole);
        }
        QStringList value = metaInfo->getMetaInfoStringList(RESTORE_ITEM_GRID_POS_ATTRIBUTE);
        item->setData(value, AdvancedDesktopIconView::ExceptedPositionRole);
        value = metaInfo->getMetaInfoStringList(RESTORE_EXTEND_ITEM_GRID_POS_ATTRIBUTE);
        item->setData(value, AdvancedDesktopIconView::ExtendScreenPositionRole);
        value = metaInfo->getMetaInfoStringList(RESTORE_SINGLESCREEN_ITEM_GRID_POS_ATTRIBUTE);
        item->setData(value, AdvancedDesktopIconView::SinglesSreenPositionRole);
    }
}

void AdvancedDesktopItemModel::getAllRestoreInfo()
{
    for (int row = 0; row < rowCount(); row++) {
        QModelIndex modelIndex = index(row, 0);
        auto uri = modelIndex.data(AdvancedDesktopIconView::UriRole).toString();
        int currentId = modelIndex.data(AdvancedDesktopIconView::ScreenIdRole).toInt();
        QVariant tmp = modelIndex.data(AdvancedDesktopIconView::ExceptedPositionRole);
        QStringList extendPos = tmp.toStringList();
       
        if (3 == extendPos.count()) {
            int col = extendPos.at(0).toInt();
            int row = extendPos.at(1).toInt();
            int id = extendPos.takeAt(2).toInt();
            if (id == currentId && col >= 0 && row >= 0) {
                 QVariant value(extendPos);
                 QPoint pos(col, row);
                 setData(modelIndex, QStringList(""), AdvancedDesktopIconView::ExceptedPositionRole);
                 setData(modelIndex, pos, AdvancedDesktopIconView::PositionRole);
            }
        }
    }
}

void AdvancedDesktopItemModel::saveExtendItemInfo()
{
    bool desktopAutoLayout = GlobalSettings::getInstance()->getValue(DESKTOP_USE_AUTO_LAYOUT).toBool();
    QVector<IconData> iconDataList;
    //task#74174 扩展屏的元素记录到metInfo,以便以后恢复
    for (int i = 0; i < rowCount(); i++) {
        QModelIndex modelIndex = index(i, 0);
        //QMap<int, QVariant>  roleData = itemData(modelIndex);
        auto position = modelIndex.data(AdvancedDesktopIconView::PositionRole).toPoint();
        int currentId = modelIndex.data(AdvancedDesktopIconView::ScreenIdRole).toInt();
        QStringList extendPos = modelIndex.data(AdvancedDesktopIconView::ExtendScreenPositionRole).toStringList();
        //恢复单屏下的图标
        if (desktopAutoLayout) {
            QString uri =  modelIndex.data(AdvancedDesktopIconView::UriRole).toString();
            iconDataList.append({currentId, position, modelIndex, uri});
        } else {
            QStringList pos = modelIndex.data(AdvancedDesktopIconView::SinglesSreenPositionRole).toStringList();
            if (pos.count() == 2) {
                setData(modelIndex, QPoint(pos[0].toInt(), pos[1].toInt()), AdvancedDesktopIconView::PositionRole);
                setData(modelIndex, QStringList(), AdvancedDesktopIconView::SinglesSreenPositionRole);
            }
        }
        //如果不存在扩展屏坐标，则保存当前坐标到扩展屏坐标
        if (extendPos.count() != 3) {
            QStringList topLeft;
            topLeft<<QString::number(position.x());
            topLeft<<QString::number(position.y());
            topLeft<<QString::number(currentId);
            setData(modelIndex, topLeft, AdvancedDesktopIconView::ExtendScreenPositionRole);
        }

        setData(modelIndex, 0, AdvancedDesktopIconView::ScreenIdRole);
    }

    if (!iconDataList.empty()) {
        // 排序
        std::sort(iconDataList.begin(), iconDataList.end());

        int newPositionIndex = 0;
        for (const auto &iconData : iconDataList) {
            setData(iconData.modelIndex, QPoint(newPositionIndex, 0), AdvancedDesktopIconView::PositionRole);
            setData(iconData.modelIndex, QStringList(), AdvancedDesktopIconView::SinglesSreenPositionRole);
            ++newPositionIndex;
        }
    }
}

void AdvancedDesktopItemModel::resetExtendItemInfo()
{
    for (int row = 0; row < rowCount(); row++) {
        QModelIndex modelIndex = index(row, 0);
        //QMap<int, QVariant>  roleData = itemData(modelIndex);
        QVariant value = modelIndex.data(AdvancedDesktopIconView::UriRole);
        QPoint currentPos = modelIndex.data(AdvancedDesktopIconView::PositionRole).toPoint();
        value = modelIndex.data(AdvancedDesktopIconView::ExtendScreenPositionRole);
        QStringList extendPos = value.toStringList();
        if (3 == extendPos.count()) {
            int col = extendPos.at(0).toInt();
            int row = extendPos.at(1).toInt();
            int id = extendPos.takeAt(2).toInt();
            if (col >= 0 && row >= 0) {
                 QPoint pos(col, row);
                 setData(modelIndex, pos, AdvancedDesktopIconView::PositionRole);
                 setData(modelIndex, id, AdvancedDesktopIconView::ScreenIdRole);
                 setData(modelIndex, QStringList(""), AdvancedDesktopIconView::ExtendScreenPositionRole);
            }
        }
        value = modelIndex.data(AdvancedDesktopIconView::SinglesSreenPositionRole);
        QStringList screenlist = value.toStringList();
        if (screenlist.count() != 2) {
            QStringList tmp;
            tmp<<QString::number(currentPos.x());
            tmp<<QString::number(currentPos.y());
            setData(modelIndex, tmp, AdvancedDesktopIconView::SinglesSreenPositionRole);
        }
    }
}

void AdvancedDesktopItemModel::clearExtendItemPos(bool saveId)
{
    //重置扩展屏和单屏坐标
    for (int row = 0; row < rowCount(); row++) {
        QModelIndex modelIndex = index(row, 0);
        //QMap<int, QVariant>  roleData = itemData(modelIndex);
        QStringList tmp("");
        setData(modelIndex, tmp, AdvancedDesktopIconView::SinglesSreenPositionRole);
        if (saveId) {
            setData(modelIndex, tmp, AdvancedDesktopIconView::ExceptedPositionRole);
            QStringList extendPos =  modelIndex.data(AdvancedDesktopIconView::ExceptedPositionRole).toStringList();
            if (extendPos.count() == 3) {
                tmp.clear();
                tmp<<"-1"<<"-1"<<extendPos[2];
            }
        }
        setData(modelIndex, tmp, AdvancedDesktopIconView::ExtendScreenPositionRole);
    }
}

void AdvancedDesktopItemModel::fileCreated(const QString &uri)
{
    qDebug()<<"desktop file created"<<uri;

    auto info = FileInfo::fromUri(uri);
    bool exsited = false;
    for (auto file : m_files) {
        if (file->uri() == info->uri()) {
            exsited = true;
            break;
        }
    }

    if (!exsited) {
        if (!m_renaming_file_pos.first.isEmpty() && uri != m_renaming_file_pos.first && uri.contains(m_renaming_file_pos.first)) {
            return;
        }
        m_items_need_relayout.append(uri);
        m_items_need_relayout.removeOne(m_renaming_file_pos.first);
        m_items_need_relayout.removeOne(m_renaming_file_pos.first + ".desktop");
        if (m_renaming_operation_info.get()) {
            m_items_need_relayout.removeOne(m_renaming_operation_info.get()->target());
        }
        m_items_need_relayout.removeDuplicates();

        auto job = new FileInfoJob(info);
        job->setAutoDelete();
        job->querySync();

        // locate new item =====
        //task#74174 扩展模式下支持拖拽图标放置到扩展屏, 创建文件获取当前view

        //file changed, force create thubnail, link tobug#83108
        ThumbnailManager::getInstance()->createThumbnail(info->uri(), m_thumbnail_watcher, true);
        m_files<<info;
        AdvancedDesktopIconItem *item = new AdvancedDesktopIconItem(info->uri(), true);
        this->appendRow(item);
        int id = item->data(AdvancedDesktopIconView::ScreenIdRole).toInt();
        auto view = PeonyDesktopApplication::getDesktopWindowManager()->getIconView(id);
        Q_EMIT view->sig_fileCreated(uri);
    }
    else{
        //file content changed, need update fileinfo, fix bug#76908
        auto job = new FileInfoJob(info);
        job->setAutoDelete();
        job->querySync();
    }
}

void AdvancedDesktopItemModel::pendingQuery(const QString &uri)
{
    // task: #355895
    // note: 指向外部分区的软链接在桌面初始化时可能查询不到链接的源文件，
    // 这可能是因为外部分区挂载比桌面拉起慢导致的，这里增加一个pending机制
    if (!m_pending_query_timer) {
        m_pending_query_timer = new QTimer(this);
        m_pending_query_timer->setInterval(1000);
        connect(m_pending_query_timer, &QTimer::timeout, this, [=]{
            for (auto uri : m_pending_query_uris) {
                // 使用现有文件改变流程触发更新
                m_desktop_watcher->fileChanged(uri);
            }
            m_pending_query_uris.clear();
        });
    }
    m_pending_query_uris.insert(uri);
    m_pending_query_timer->start();
}

void AdvancedDesktopItemModel::saveExtendItemInfo(int id)
{
    //task#74174 销毁时保存扩展屏元素的坐标点
    for (int row = 0; row < rowCount(); row++) {
        QModelIndex modelIndex = index(row, 0);
        int currentId = modelIndex.data(AdvancedDesktopIconView::ScreenIdRole).toInt();
        if(currentId != id) {
            continue;
        }
        //QMap<int, QVariant>  roleData = itemData(modelIndex);
        auto uri = modelIndex.data(AdvancedDesktopIconView::UriRole).toString();
        QVariant tmp = modelIndex.data(AdvancedDesktopIconView::ExceptedPositionRole);
        QStringList extendPos = tmp.toStringList();
        if (3 != extendPos.count() && currentId != extendPos[2].toInt()) {
            QPoint pos = modelIndex.data(AdvancedDesktopIconView::PositionRole).toPoint();
            QStringList topLeft;
            topLeft<<QString::number(pos.x());
            topLeft<<QString::number(pos.y());
            topLeft<<QString::number(id);
            extendPos = topLeft;
        }
        setData(modelIndex, extendPos, AdvancedDesktopIconView::ExtendScreenPositionRole);
        QStringList singlesScreen = modelIndex.data(AdvancedDesktopIconView::SinglesSreenPositionRole).toStringList();
        if(2 == singlesScreen.count()) {
            int col = extendPos.at(0).toInt();
            int row = extendPos.at(1).toInt();
            QPoint pos(col, row);
            setData(modelIndex, pos, AdvancedDesktopIconView::PositionRole);
        }
    }
}
