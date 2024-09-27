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
 * Authors: yangyanwei <yangyanwei@kylinos.cn>
 *
 */

#include <QApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QRubberBand>
#include <QMouseEvent>
#include <QStandardItem>
#include <QDebug>
#include <QMessageBox>
#include <QClipboard>
#include <QDrag>
#include <QtX11Extras/QX11Info>
#include <kstartupinfo.h>
#include <QProcess>
#include <QToolTip>
#include <QWindow>
#include <QDrag>

#include "advanced-desktop-icon-view.h"
#include "file-enumerator.h"
#include "file-meta-info.h"
#include "file-info.h"
#include "file-info-job.h"
#include "file-info-manager.h"
#include "file-watcher.h"
#include "file-operation-manager.h"
#include "file-move-operation.h"
#include "file-trash-operation.h"
#include "file-copy-operation.h"
#include "file-operation-utils.h"
#include "file-utils.h"

#include "thumbnail-manager.h"
#include "usershare-manager.h"

#include "global-settings.h"
#include "sound-effect.h"
#include "audio-play-manager.h"

#include "desktop-item-proxy-model.h"
#include "advanced-desktop-item-model.h"
#include "peony-desktop-application.h"

#include "icon-view-style.h"
#include "desktop-icon-view-delegate.h"
#include "clipboard-utils.h"

#include "properties-window.h"

#include "desktop-menu.h"

#include "file-item-model.h"
#include "file-launch-manager.h"

#include "desktop-index-widget.h"
#include "common.h"

using namespace Peony;

static bool refreshing = false;

static bool meetSpecialConditions(const QStringList& selectedUris)
{
    /* The desktop home directory, computer, and trash do not allow operations such as copying, cutting,
     * deleting, renaming, moving, or using shortcut keys for corresponding operations.add by 2021/06/17 */
    static QString homeUri = "file://" +  QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    if (selectedUris.contains("computer:///")
       ||selectedUris.contains("trash:///")
       ||selectedUris.contains(homeUri)){
        return true;
    }

    return false;
}

AdvancedDesktopIconView::AdvancedDesktopIconView(QWidget *parent) : QAbstractItemView(parent)
{

    QString localeName = QLocale::system().name();
    if (localeName.contains("ug") || localeName.contains("kk") || localeName.contains("ky")) {
         setLayoutDirection(Qt::RightToLeft);
    }

    setAttribute(Qt::WA_AlwaysStackOnTop);
    setFrameShape(QFrame::NoFrame);
    setStyleSheet("QAbstractItemView { background-color: transparent; }");
    setItemDelegate(new DesktopIconViewDelegate(this));

    auto zoomLevel = this->zoomLevel();
    setDefaultZoomLevel(zoomLevel);

    initShoutCut();
    initDoubleClick();

    connect(qApp, &QApplication::paletteChanged, this, [=]() {
        viewport()->update();
    });

    auto globalSettings = Peony::GlobalSettings::getInstance();
    QString widgetThemeName = globalSettings->getValue("widgetThemeName").toString();
    if (widgetThemeName.contains("classical")) {
        m_radius = 0;
    } else {
        m_radius = 6;
    }
    connect(globalSettings, &GlobalSettings::valueChanged, this, [=](const QString &key){
        if (key == "widgetThemeName") {
            QString widgetThemeName = globalSettings->getValue("widgetThemeName").toString();
            if (widgetThemeName.contains("classical")) {
                m_radius = 0;
            } else {
                m_radius = 6;
            }
            viewport()->update();
        }
    });

    m_edit_trigger_timer.setSingleShot(true);
    m_edit_trigger_timer.setInterval(3000);
    m_last_index = QModelIndex();

    // rubberband
    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this/*->viewport()*/);
    m_rubberBand->setVisible(false);

    m_model = PeonyDesktopApplication::getModel();
    m_proxy_model = new DesktopItemProxyModel(m_model);

    m_proxy_model->setSourceModel(m_model);

    setModel(m_proxy_model);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::DragDrop);
    setMouseTracking(true);//追踪鼠标
    setEditTriggers(QListView::NoEditTriggers);

    // dnd
    setDefaultDropAction(Qt::MoveAction);

    m_peonyDbusSer = new PeonyDbusService(this);
    m_peonyDbusSer->DbusServerRegister();

    setMouseTracking(true);//追踪鼠标

    connect(this, &AdvancedDesktopIconView::updateView, this, [=]() {
        m_itemPosHash.clear();
        m_autoArrange.clear();
        m_resolutionItemPosHash.clear();
        m_proxy_model->invalidate();

        reset();
        if (m_proxy_model && m_proxy_model->getDesktopUseAutoLayout()) {
            if (m_autoArrange.count() != m_proxy_model->rowCount()) {
                qInfo()<<"desktop use auto layout but config cache is invalid, use current layout";
                auto sortedUris = layoutItems();
//                relayoutExsitingItemsAndUpdate(sortedUris);
            }
            return;
        }
        resolutionChange();
        checkItemsOver();
        repaint();
    });

    connect(m_model, &AdvancedDesktopItemModel::refreshed, this, [=]() {
        this->setCursor(QCursor(Qt::ArrowCursor));
        refreshing = false;
        if (m_proxy_model->getDesktopUseAutoLayout()) {
            if (m_autoArrange.count() != m_proxy_model->rowCount()) {
                qInfo()<<"desktop use auto layout but config cache is invalid, use current layout";
                auto sortedUris = layoutItems();
                m_autoArrange = sortedUris;
            }
            viewport()->update();
            return;
        }
        checkItemsOver();
        viewport()->update();
    });

    // 图标和字体共同决定网格
    connect(this, &AdvancedDesktopIconView::iconSizeChanged, this, &AdvancedDesktopIconView::recalculateAvailableRowAndColumnCount);

    connect(qApp, &QApplication::fontChanged, this, [=](){
        //直接调用槽函数，获取font的高度是没改变之前的
        QTimer::singleShot(0, this, [this]() {
            recalculateAvailableRowAndColumnCount();
        });
    });

    connect(m_model, &AdvancedDesktopItemModel::requestClearIndexWidget, this, &AdvancedDesktopIconView::clearAllIndexWidgets);

    connect(GlobalSettings::getInstance(), &GlobalSettings::valueChanged, this, [=] (const QString &key) {
        if (key == DISPLAY_STANDARD_ICONS ||
            key == HOME_ICON_VISIBLE ||
            key == TRASH_ICON_VISIBLE ||
            key == COMPUTER_ICON_VISIBLE) {
            //this->refresh();
            //m_proxy_model->invalidate();
            m_proxy_model->invalidateModel();
            this->resolutionChange();
            checkItemsOver();
        } else if (SHOW_HIDDEN_PREFERENCE == key) {
            m_show_hidden= GlobalSettings::getInstance()->getValue(key).toBool();
        }
    });


    //fix task bar overlap with desktop icon and can drag move issue
    //bug #27811,33188
    if (QGSettings::isSchemaInstalled(PANEL_SETTINGS))
    {
        //panel monitor
        if (!m_panelSetting)
            m_panelSetting = new QGSettings(PANEL_SETTINGS, QByteArray(), this);
        connect(m_panelSetting, &QGSettings::changed, this, [=](const QString &key){
            if (key == "panelposition" || key == "panelsize" || key == "settingsislandposition" || key == "paneltype") {
                setMargins();
                recalculateAvailableRowAndColumnCount();
            }
        });
        setMargins();
    }

    // try fixing #63358
    if (QGSettings::isSchemaInstalled(UKUI_STYLE_SETTINGS)) {
        auto styleSettings = new QGSettings(UKUI_STYLE_SETTINGS, QByteArray(), this);
        connect(styleSettings, &QGSettings::changed, this, [=](const QString &key){
            if (key == "iconThemeName") {
                QTimer::singleShot(1000, viewport(), [=]{
                    viewport()->update();
                });
            }
        });
    }
    connect(m_model, &AdvancedDesktopItemModel::sig_relayoutItems, this, [=](){
        if (m_proxy_model->getDesktopUseAutoLayout() && !m_is_renaming) {
             auto list = m_autoArrange;
             relayoutExsitingItemsAndUpdate(list);
        }
    });

    connect(m_model, &AdvancedDesktopItemModel::selectUri, this, [=](const QString &uri){
        if (!m_itemPosHash.contains(uri)) {
            return;
        }
        QTimer::singleShot(100, this, [=]() {
            setSelections(QStringList() << uri);
            scrollToSelection(uri);
            setFocus();
        });
    });

    connect(this, &AdvancedDesktopIconView::sig_fileCreated, this, &AdvancedDesktopIconView::fileCreated);

    bool desktopAutoLayout = GlobalSettings::getInstance()->getValue(DESKTOP_USE_AUTO_LAYOUT).toBool();
    m_proxy_model->setDesktopUseAutoLayout(desktopAutoLayout);
    setDropIndicatorShown(desktopAutoLayout);
    connect(GlobalSettings::getInstance(), &GlobalSettings::valueChanged, this, [=](const QString &key){
        if (key == DESKTOP_USE_AUTO_LAYOUT) {
            bool desktopAutoLayout = GlobalSettings::getInstance()->getValue(DESKTOP_USE_AUTO_LAYOUT).toBool();
            m_proxy_model->setDesktopUseAutoLayout(desktopAutoLayout);
            setDropIndicatorShown(desktopAutoLayout);
            if (desktopAutoLayout) {
                auto sortedUris = layoutItems();
                m_proxy_model->setSortedUris(sortedUris);
            } else {
                m_proxy_model->setSortedUris(QStringList());
            }
        }
    });

    connect(this, &QAbstractItemView::entered, this, &AdvancedDesktopIconView::onEntered);
    connect(m_proxy_model, &DesktopItemProxyModel::showHiddenFile, this, &AdvancedDesktopIconView::showHiddenFile);
}

AdvancedDesktopIconView::~AdvancedDesktopIconView()
{
    delete m_peonyDbusSer;
    //saveAllItemPosistionInfos();
}

void AdvancedDesktopIconView::initShoutCut()
{
    QAction *copyAction = new QAction(this);
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, [=]() {
        if (! GlobalSettings::getInstance()->getValue(ENABLE_SHORTCUT_KEYS).toBool()) {
            return ;
        }

        auto selectedUris = this->getSelections();
        if (!selectedUris.isEmpty() && !meetSpecialConditions(selectedUris)){
            ClipboardUtils::setClipboardFiles(selectedUris, false);
            this->viewport()->update();
        }
    });
    addAction(copyAction);

    QAction *cutAction = new QAction(this);
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, [=]() {
        if (! GlobalSettings::getInstance()->getValue(ENABLE_SHORTCUT_KEYS).toBool()) {
            return ;
        }

        auto selectedUris = this->getSelections();
        if (!selectedUris.isEmpty() && !meetSpecialConditions(selectedUris))
        {
            ClipboardUtils::setClipboardFiles(selectedUris, true);
            //this->update();
            this->viewport()->update();
        }
    });
    addAction(cutAction);

    QAction *pasteAction = new QAction(this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, [=]() {
        if (! GlobalSettings::getInstance()->getValue(ENABLE_SHORTCUT_KEYS).toBool()) {
            return ;
        }

        if (qApp->clipboard()->mimeData()->hasFormat ("uos/remote-copy")) {
            auto op = ClipboardUtils::pasteClipboardFiles(this->getDirectoryUri());
            if (!op) {
                viewport()->update();
            }
        } else {
            //auto clipUris = ClipboardUtils::getClipboardFilesUris();
            if (ClipboardUtils::getInstance()->isClipboardHasFiles() && !meetSpecialConditions(this->getSelections())) {
                auto op = ClipboardUtils::pasteClipboardFiles(this->getDirectoryUri());
                if (!op) {
                    viewport()->update();
                }
            }
        }
    });
    addAction(pasteAction);

    //add CTRL+D for delete operation
    auto trashAction = new QAction(this);
    trashAction->setShortcuts(QList<QKeySequence>()<<Qt::Key_Delete<<QKeySequence(Qt::CTRL + Qt::Key_D));
    connect(trashAction, &QAction::triggered, [=]() {
        if (! GlobalSettings::getInstance()->getValue(ENABLE_SHORTCUT_KEYS).toBool()) {
            return ;
        }

        auto selectedUris = getSelections();
        if (!selectedUris.isEmpty() && !meetSpecialConditions(selectedUris)){
           FileOperationUtils::trash(selectedUris, true);
        }
    });
    addAction(trashAction);

    QAction *undoAction = new QAction(this);
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered,
    [=]() {
        // do not relayout item with undo.
        setRenaming(true);
        FileOperationManager::getInstance()->undo();
    });
    addAction(undoAction);

    QAction *redoAction = new QAction(this);
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered,
    [=]() {
        // do not relayout item with redo.
        setRenaming(true);
        FileOperationManager::getInstance()->redo();
    });
    addAction(redoAction);

    QAction *zoomInAction = new QAction(this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, [=]() {
        this->zoomIn();
    });
    addAction(zoomInAction);

    QAction *zoomOutAction = new QAction(this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, [=]() {
        this->zoomOut();
    });
    addAction(zoomOutAction);

    QAction *renameAction = new QAction(this);
    renameAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_E));
    connect(renameAction, &QAction::triggered, [=]() {
        auto selections = this->getSelections();
        if (selections.count() == 1 && !meetSpecialConditions(selections)) {
            this->editUri(selections.first());
        }
    });
    addAction(renameAction);

    QAction *removeAction = new QAction(this);
    removeAction->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete));
    connect(removeAction, &QAction::triggered, [=]() {
        if (! GlobalSettings::getInstance()->getValue(ENABLE_SHORTCUT_KEYS).toBool()) {
            return ;
        }

        auto selectedUris = this->getSelections();
        if (!meetSpecialConditions(selectedUris)){
            qDebug() << "delete" << selectedUris;
            clearAllIndexWidgets();
            FileOperationUtils::executeRemoveActionWithDialog(selectedUris);
        }
    });
    addAction(removeAction);

    QAction *helpAction = new QAction(this);
    helpAction->setShortcut(Qt::Key_F1);
    connect(helpAction, &QAction::triggered, this, [=]() {
        PeonyDesktopApplication::showGuide();
    });
    addAction(helpAction);

    auto propertiesWindowAction = new QAction(this);
    propertiesWindowAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::ALT + Qt::Key_Return)
                                         <<QKeySequence(Qt::ALT + Qt::Key_Enter));
    connect(propertiesWindowAction, &QAction::triggered, this, [=]() {
        DesktopMenu menu(this);
        if (this->getSelections().count() > 0)
        {
            menu.showProperties(this->getSelections());
        }
        else
        {
            QString desktopPath = "file://" +  QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
            menu.showProperties(desktopPath);
        }
    });
    addAction(propertiesWindowAction);

    auto newFolderAction = new QAction(this);
    newFolderAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));
    connect(newFolderAction, &QAction::triggered, this, [=]() {
        CreateTemplateOperation op(this->getDirectoryUri(), CreateTemplateOperation::EmptyFolder, tr("New Folder"));
        op.run();
        auto targetUri = op.target();

        QTimer::singleShot(300, this, [=]() {
            this->scrollToSelection(targetUri);

            for (auto index : m_proxy_model->getAllFileIndexes()) {
                closePersistentEditor(index);
                setIndexWidget(index, nullptr);
            }
            selectionModel()->clearSelection();

            setState(QListView::NoState);
            auto origin = FileUtils::getOriginalUri(targetUri);
            auto index = m_proxy_model->mapFromSource(m_model->indexFromUri(origin));
            edit(index);
        });
    });
    addAction(newFolderAction);

    QAction *refreshWinAction = new QAction(this);
    refreshWinAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(refreshWinAction, &QAction::triggered, [=]() {
        this->refresh();
    });
    addAction(refreshWinAction);

    QAction *reverseSelectAction = new QAction(this);
    reverseSelectAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L));
    connect(reverseSelectAction, &QAction::triggered, [=]() {
        this->invertSelections();
    });
    addAction(reverseSelectAction);

    QAction *normalIconAction = new QAction(this);
    normalIconAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    connect(normalIconAction, &QAction::triggered, [=]() {
        if (this->zoomLevel() == AdvancedDesktopIconView::Normal)
            return;
        this->setDefaultZoomLevel(AdvancedDesktopIconView::Normal);
    });
    addAction(normalIconAction);

    auto refreshAction = new QAction(this);
    refreshAction->setShortcut(Qt::Key_F5);
    connect(refreshAction, &QAction::triggered, this, [=]() {
        this->refresh();
    });
    addAction(refreshAction);

    QAction *editAction = new QAction(this);
    editAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::ALT + Qt::Key_E)<<Qt::Key_F2);
    connect(editAction, &QAction::triggered, this, [=]() {
        auto selections = this->getSelections();
        if (selections.count() == 1) {
            this->editUri(selections.first());
        } else if (selections.count() > 1) {
            this->editUris(selections);
        }
    });
    addAction(editAction);

    auto settings = GlobalSettings::getInstance();
    m_show_hidden = settings->isExist(SHOW_HIDDEN_PREFERENCE)? settings->getValue(SHOW_HIDDEN_PREFERENCE).toBool(): false;
    //show hidden action
    QAction *showHiddenAction = new QAction(this);
    showHiddenAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    addAction(showHiddenAction);
    connect(showHiddenAction, &QAction::triggered, this, [=]() {
        this->setShowHidden();
    });

    auto cancelAction = new QAction(this);
    cancelAction->setShortcut(Qt::Key_Escape);
    connect(cancelAction, &QAction::triggered, [=]() {
        if (Peony::ClipboardUtils::isClipboardHasFiles())
        {
            Peony::ClipboardUtils::clearClipboard();
            this->update();
        }
    });
    addAction(cancelAction);

    auto *selectAllAction = new QAction(this);
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, this, [=]() {
        if (! GlobalSettings::getInstance()->getValue(ENABLE_SHORTCUT_KEYS).toBool()) {
            return ;
        }

        this->selectAll();
    });
    addAction(selectAllAction);
}

void AdvancedDesktopIconView::openFileByUri(QString uri)
{
    auto info = FileInfo::fromUri(uri);
    auto job = new FileInfoJob(info);
    job->setAutoDelete();
    job->connect(job, &FileInfoJob::queryAsyncFinished, [=]() {
        if ((info->isDir() || info->isVolume() || info->isVirtual())) {
            QDir dir(info->filePath());
            if (! dir.exists())
            {
                Peony::AudioPlayManager::getInstance()->playWarningAudio();
                auto result = QMessageBox::question(nullptr, tr("Open Link failed"),
                                      tr("File not exist, do you want to delete the link file?"),
                                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                if (result == QMessageBox::Yes) {
                    qDebug() << "Delete unused symbollink in desktop.";
                    QStringList selections;
                    selections.push_back(uri);
                    FileOperationUtils::trash(selections, true);
                }
                return;
            }

            if (! info->uri().startsWith("trash://")
                    && ! info->uri().startsWith("computer://")
                    &&  ! info->canExecute())
            {
                Peony::AudioPlayManager::getInstance()->playWarningAudio();
                QMessageBox::critical(nullptr, tr("Open failed"),
                                      tr("Open directory failed, you have no permission!"));
                return;
            }

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            QProcess p;
            QUrl url = uri;
            p.setProgram("peony");
            p.setArguments(QStringList() << url.toEncoded() <<"%U&");
            qint64 pid;
            p.startDetached(&pid);

            // send startinfo to kwindowsystem
            quint32 timeStamp = QX11Info::isPlatformX11() ? QX11Info::appUserTime() : 0;
            KStartupInfoId startInfoId;
            startInfoId.initId(KStartupInfo::createNewStartupIdForTimestamp(timeStamp));
            startInfoId.setupStartupEnv();
            KStartupInfoData data;
            data.setHostname();
            data.addPid(pid);
            QRect rect = info.get()->property("iconGeometry").toRect();
#ifdef KSTARTUPINFO_HAS_SET_ICON_GEOMETRY
            if (rect.isValid())
                data.setIconGeometry(rect);
#endif
            data.setLaunchedBy(getpid());
            KStartupInfo::sendStartup(startInfoId, data);
#else
            QProcess p;
            QString strq;
            for (int i = 0;i < uri.length();++i) {
                if(uri[i] == ' '){
                    strq += "%20";
                }else{
                    strq += uri[i];
                }
            }

            p.startDetached("/usr/bin/peony", QStringList()<<strq<<"%U&");
#endif
        } else {
            if (!(info->isDesktopFile() && execSharedFileLink(uri))) {
                FileLaunchManager::openAsync(uri, false, false);
            }
        }
        this->clearSelection();
    });
    job->queryAsync();
}

void AdvancedDesktopIconView::initDoubleClick()
{
    connect(this, &QAbstractItemView::activated, this, [=](const QModelIndex &index) {
        qDebug() << "double click" << index.data(FileItemModel::UriRole);
        auto uri = index.data(FileItemModel::UriRole).toString();
        openFileByUri(uri);
    }, Qt::UniqueConnection);
}

void AdvancedDesktopIconView::setMargins()
{
    int settingsislandposition = m_panelSetting->get("settingsislandposition").toInt();
    int paneltype = m_panelSetting->get("paneltype").toInt();
    int position = m_panelSetting->get("panelposition").toInt();
    int margins = m_panelSetting->get("panelsize").toInt();

    if (settingsislandposition == 1 && paneltype == 1) {
        setViewportMargins(0, 32, 0, margins);
        return;
    }
    switch (position) {
    case 1: {
        setViewportMargins(0, margins, 0, 0);
        break;
    }
    case 2: {
        setViewportMargins(margins, 0, 0, 0);
        break;
    }
    case 3: {
        setViewportMargins(0, 0, margins, 0);
        break;
    }
    default: {
        setViewportMargins(0, 0, 0, margins);
        break;
    }
    }
}

void AdvancedDesktopIconView::setId(int id)
{
    m_proxy_model->setId(id);
    m_id = id ;
}

QRect AdvancedDesktopIconView::visualRect(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRect();

    QRect rect = QRect(QPoint(0, 0), m_gridSize);
    rect.adjust(5, 5, -5, -5);
    // todo: 通过model data获取index位置
    bool ok = false;
    auto pos = getIndexGridPos(index, &ok);
    if (ok) {
        //qDebug()<<index.data()<<pos;
        rect.moveTo(pos.x() * m_gridSize.width() + m_margin.x(), pos.y() * m_gridSize.height() + m_margin.y());
        if (layoutDirection() == Qt::RightToLeft) {
            int vewportX = viewport()->rect().topRight().x();
            int x = vewportX - rect.topRight().x() ;
            rect = QRect(x, rect.y(), rect.width(), rect.height());
        }
    } else {
        qWarning()<<"no pos for item"<<index.data()<<pos;
    }

    return rect;
}

void AdvancedDesktopIconView::scrollTo(const QModelIndex &index, QAbstractItemView::ScrollHint hint)
{
    Q_UNUSED(index)
    Q_UNUSED(hint)
    return;
}

QModelIndex AdvancedDesktopIconView::indexAt(const QPoint &point) const
{
    // todo: 通过model data获取index位置，或者通过缓存获取
    for (int i = 0; i < model()->rowCount(); i++) {
        auto index = model()->index(i, 0);
        auto tmpRect = visualRect(index);
        if (tmpRect.contains(point))
            return index;
    }
    return QModelIndex();
}

QModelIndex AdvancedDesktopIconView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    // todo: 实现键盘切换选择
    return QModelIndex();
}

int AdvancedDesktopIconView::horizontalOffset() const
{
    return 0;
}

int AdvancedDesktopIconView::verticalOffset() const
{
    return 0;
}

bool AdvancedDesktopIconView::isIndexHidden(const QModelIndex &index) const
{
    // todo: 根据屏幕id匹配
    bool isHidden = /*m_id == index.data(ScreenIdRole).toInt() ? true :*/ false;
    return isHidden;
}

// fixme: 非qlistview不能在index非法的情况下触发选择，需要在mouseEvent中处理此流程
void AdvancedDesktopIconView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    qDebug()<<"set selection"<<rect<<command;
    auto tmpRect = rect.normalized();
    QItemSelection selection;
    if (tmpRect.isEmpty())
        return;

    bool notMultiSelection = (rect.width() == 1 && rect.height() == 1) || state() == DragSelectingState ;
    if (m_shift_key_pressed && !notMultiSelection) {
        //shift 按键连选，获取首尾相对坐标，中间的所有项都选中
        QPoint leftPos ,rightPos;
        auto leftIndex = indexAt(tmpRect.topLeft());
        bool leftOk = false;
        leftPos = getIndexGridPos(leftIndex, &leftOk);
        auto rightIndex= indexAt(tmpRect.bottomRight());
        bool rightOk = false;
        rightPos = getIndexGridPos(rightIndex, &rightOk);

        if (leftOk && rightOk) {
            for (int i = 0; i < model()->rowCount(); i++) {
            auto index = model()->index(i, 0);
            bool ok = false;
            auto pos = getIndexGridPos(index, &ok);

            if (gridPosLesserThan(pos, leftPos)|| gridPosLesserThan(rightPos, pos)) {
                continue;
            }
            selection.select(index, index);
            // 单击重叠的图标只选中一个
            if (command == QItemSelectionModel::ClearAndSelect || command == QItemSelectionModel::Select)
                break;
            }

            selectionModel()->select(selection, command);
            return;
        }
    }

    // todo 性能优化
    for (int i = 0; i < model()->rowCount(); i++) {
        auto index = model()->index(i, 0);
        auto indexRect = visualRect(index);
        if (!indexRect.intersects(tmpRect) || !index.isValid())
            continue;
        selection.select(index, index);
        // 单击重叠的图标只选中一个
        if (command == QItemSelectionModel::ClearAndSelect || command == QItemSelectionModel::Select)
            break;
    }

    selectionModel()->select(selection, command);
}

QRegion AdvancedDesktopIconView::visualRegionForSelection(const QItemSelection &selection) const
{
    QRegion region;
    auto indexes = selection.indexes();
    for (auto index : indexes) {
        if (!index.isValid())
            continue;
        auto tmpRect = visualRect(index);
        //防止分数缩放和显示覆盖影响
        region += tmpRect.adjusted(-5, -5, 5, 5);
    }
    region.intersects(this->viewport()->rect());
    return region;
}

QStyleOptionViewItem AdvancedDesktopIconView::viewOptions() const
{
    auto option = QAbstractItemView::viewOptions();
    option.decorationAlignment = Qt::AlignTop|Qt::AlignHCenter;
    option.decorationPosition = QStyleOptionViewItem::Top;
    option.displayAlignment = Qt::AlignTop|Qt::AlignHCenter;
    option.textElideMode = Qt::ElideMiddle;
    option.features = QStyleOptionViewItem::HasDisplay|QStyleOptionViewItem::HasDecoration|QStyleOptionViewItem::WrapText;
    return option;
}

void AdvancedDesktopIconView::startDrag(Qt::DropActions supportedActions)
{
    // fixme默认为moveaction，支持copyaction
    m_rubberBand->setVisible(false);

    auto indexes = selectedIndexes();
    if (indexes.count() > 0) {
        auto pos = m_dragPressPos;
        qreal scale = 1.0;
        QWidget *window = this->window();
        if (window) {
            auto windowHandle = window->windowHandle();
            if (windowHandle) {
                scale = windowHandle->devicePixelRatio();
            }
        }

        auto drag = new QDrag(this);
        drag->setMimeData(model()->mimeData(indexes));

        QRegion rect;
        QHash<QModelIndex, QRect> indexRectHash;
        for (auto index : indexes) {
            rect += (visualRect(index));
            indexRectHash.insert(index, visualRect(index));
        }

        QRect realRect = rect.boundingRect();

        // fix #78263, text displayment is not completed.
        //realRect.adjust(-5, -5, 5, 5);

        realRect.adjust(-15, -15, 15, 15);
        QPixmap pixmap(realRect.size() * scale);
        pixmap.fill(Qt::transparent);
        pixmap.setDevicePixelRatio(scale);
        QPainter painter(&pixmap);
        // try fixing #190315, text shadow displayment issue while compositing not running.
        bool shouldDrawBackground = !QX11Info::isCompositingManagerRunning();
        for (auto index : indexes) {
            painter.save();
            painter.translate(indexRectHash.value(index).topLeft() - rect.boundingRect().topLeft());
            if (shouldDrawBackground) {
                painter.setPen(qApp->palette().highlight().color());
                painter.setBrush(qApp->palette().highlight());
                painter.drawRoundedRect(QRect(0, 0, m_gridSize.width(), m_gridSize.height()).adjusted(1, 1, -1, -1), 6, 6);
            }
            QStyleOptionViewItem opt = viewOptions();
            auto viewItemDelegate = qobject_cast<DesktopIconViewDelegate *>(itemDelegate());
            viewItemDelegate->initIndexOption(&opt, index);
            opt.rect.setSize(visualRect(index).size());
            itemDelegate()->paint(&painter, opt, index);
            painter.restore();
        }

        drag->setPixmap(pixmap);
        drag->setHotSpot(pos - rect.boundingRect().topLeft() - QPoint(viewportMargins().left(), viewportMargins().top()));
        drag->setDragCursor(QPixmap(), m_ctrl_key_pressed? Qt::CopyAction: Qt::MoveAction);
        drag->exec(m_ctrl_key_pressed? Qt::CopyAction: Qt::MoveAction);

    } else {
        return QAbstractItemView::startDrag(Qt::MoveAction|Qt::CopyAction);
    }

}

void AdvancedDesktopIconView::reset()
{
    // todo 待优化
    if (m_availableColumnCount == 1 && m_availableRowCount == 1)
        return;
    if (model() && m_itemPosHash.count() < model()->rowCount()) {
        bool ok = false;
        qDebug()<<model()->rowCount();
        for (int row = 0; row < model()->rowCount(); row++) {
            // todo: 桌面整理时的表现
            auto index = model()->index(row, 0, QModelIndex());
            auto pos = getIndexGridPos(index, &ok);
            if (!ok) {
                auto emptypos = findNextEmptyGridPos();
                if (emptypos.x() < 0) {
                    qWarning()<<"no empty pos";
                    //如果没有空位则后面的item不排序
                    break;
                }
                pos = emptypos;
            }
            auto uri = index.data(UriRole).toString();
            model()->setData(index, pos, PositionRole);
            setFileMetaInfoPos(uri, pos);
        }
    }
    QAbstractItemView::reset();
}

void AdvancedDesktopIconView::rowsInserted(const QModelIndex &parent, int start, int end)
{
//    // 如果没有找到排序配置，则跳过此流程，避免出现顺序异常改变，这可能出现在首次升级且异常关闭的场景
    bool ok = false;
    bool isFull = false;
    QPoint pos = QPoint(0, 0);
    if (m_proxy_model->getDesktopUseAutoLayout()) {
        QAbstractItemView::rowsInserted(parent, start, end);
        for (int i = start; i <= end; i++) {
            auto index = model()->index(i, 0, parent);
            auto uriAdded = index.data(Qt::UserRole).toString();
            if (!m_autoArrange.contains(uriAdded)) {
                bool isRenaming = false;
                if (!isFull) {
                    pos = getIndexGridPos(index, &ok);
                    if (!ok || pos == QPoint(-1,-1) || (!refreshing && !m_itemPosHash.keys(pos).isEmpty())) {
                        isRenaming = true;
                        auto emptypos = findNextEmptyGridPos();
                        if (emptypos.x() < 0) {
                            qWarning()<<"no empty pos";
                            //如果没有空位则后面的item不排序
                            emptypos = QPoint(0, 0);
                            if (!m_storageBox.contains(uriAdded))
                                m_storageBox.append(uriAdded);
                            isFull = true;
                        }
                        pos = emptypos;
                    }
                }
                model()->setData(index, pos, PositionRole);
                setFileMetaInfoPos(uriAdded, pos);
                if (!refreshing) {
                    if (!isRenaming) {
                        int index = pos.x() * m_availableRowCount +  pos.y();
                        m_autoArrange.insert(index, uriAdded);
                    } else {
                        //刷新的时候不是按照顺序加载，统一在刷新之后获取列表
                        m_autoArrange.append(uriAdded);
                    }
                }
            }
        }
        return;
    }

    for (int row = start; row <= end; row++) {
        // todo: 桌面整理时的表现
        auto index = model()->index(row, 0, parent);
        QString uri = index.data(UriRole).toString();
        m_itemPosHash.remove(uri);

        if (!isFull) {
            pos = getIndexGridPos(index, &ok);

            if (!ok || pos == QPoint(-1,-1) || (!refreshing && !m_itemPosHash.keys(pos).isEmpty())) {
                auto emptypos = findNextEmptyGridPos();
                if (emptypos.x() < 0) {
                    qWarning()<<"no empty pos";
                    //如果没有空位则后面的item不排序
                    emptypos = QPoint(0, 0);
                    if (!m_storageBox.contains(uri))
                        m_storageBox.append(uri);
                    isFull = true;
                }
                pos = emptypos;
            }
        }
        qDebug() << "-----------" <<m_itemPosHash;
        model()->setData(index, pos, PositionRole);
        setFileMetaInfoPos(uri, pos);
    }

    QAbstractItemView::rowsInserted(parent, start, end);
}

void AdvancedDesktopIconView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    for (int row = start; row <= end; row++) {
        auto index = model()->index(row, 0);
        auto uri = index.data(Qt::UserRole).toString();
        m_itemPosHash.remove(uri);
        m_resolutionItemPosHash.remove(uri);
        m_autoArrange.removeOne(uri);
        m_storageBox.removeOne(uri);
        qDebug() <<"AdvancedDesktopIconView::rowsAboutToBeRemoved" <<uri;
    }

    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
    clearAllIndexWidgets();
}

void AdvancedDesktopIconView::paintEvent(QPaintEvent *e)
{
    QPainter painter(this->viewport());
    //QRect renderRect = e->rect();
    QRect renderRect = viewport()->rect();
    // todo: caculate item to be rendered.
    int num = model()->rowCount();
    for (int i = 0; i < model()->rowCount(); i++) {
        auto options = viewOptions();
        auto index = model()->index(i, 0);
        if (selectionModel()->selectedIndexes().contains(index))
            options.state |= QStyle::State_Selected;
        auto tmpRect = visualRect(index);

        options.state.setFlag(QStyle::State_MouseOver, m_hoverIndex == index);
        options.rect = tmpRect;
        auto delegate = qobject_cast<QStyledItemDelegate*>(itemDelegate());
        delegate->paint(&painter, options, index);
    }
    if ((state() == DraggingState || m_isDragging) && m_proxy_model->getDesktopUseAutoLayout()) {
        QPainter p(viewport());
        QStyleOption opt;
        opt.init(this);
        opt.rect = m_dropIndicatorRect;
        opt.palette.setColor(QPalette::Highlight, opt.palette.highlightedText().color());
        style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, &p, this);
    }
}

void AdvancedDesktopIconView::refresh()
{
    this->setCursor(QCursor(Qt::WaitCursor));
    //fix refresh clear copy files issue, link to bug#109247
    if (Peony::ClipboardUtils::isDesktopFilesBeCut())
        Peony::ClipboardUtils::clearClipboard();/* Refresh clear cut status */
    if (!m_model)
        return;

    if (refreshing)
        return;
    refreshing = true;
    m_itemPosHash.clear();
    m_autoArrange.clear();
    m_model->refresh();
}

void AdvancedDesktopIconView::resizeEvent(QResizeEvent *e)
{
    QAbstractItemView::resizeEvent(e);
    // todo: 批处理
    recalculateAvailableRowAndColumnCount();
    // todo: 通过model data获取index位置，或者通过缓存获取
    this->viewport()->update();
}

void AdvancedDesktopIconView::dropEvent(QDropEvent *event)
{
    m_isDragging = false;
    m_rubberBand->setVisible(false);
    // fix #122768, dirty region issues.
    this->viewport()->update();
    m_real_do_edit = false;
    //qDebug()<<"drop event";
    /*!
      \todo
      fix the bug that move drop action can not move the desktop
      item to correct position.

      i use copy action to avoid this bug, but the drop indicator
      is incorrect.
      */
    m_edit_trigger_timer.stop();
    if (event->keyboardModifiers() & Qt::ControlModifier) {
        m_ctrl_key_pressed = true;
    } else {
        m_ctrl_key_pressed = false;
    }

    auto action = m_ctrl_key_pressed ? Qt::CopyAction : Qt::MoveAction;
    if (event->keyboardModifiers() & Qt::ShiftModifier) {
        action = Qt::TargetMoveAction;
    }
    qDebug() << "DesktopIconView dropEvent" <<action;
    auto view = qobject_cast<AdvancedDesktopIconView *>(event->source());
    if (m_proxy_model->getDesktopUseAutoLayout() && m_dropIndicatorPos != DropIndicatorPosition::OnItem && !m_ctrl_key_pressed) {
        qDebug()<<"drop do auto layout move";
        bool sucess = dropWhenAotoArrange(event);
        if (sucess) {
            m_pressedPos = QPoint(-1,-1);
            view->m_pressedPos = QPoint(-1,-1);
            return;
        }
    }

    auto pos = event->pos();
    auto index = indexAt(pos);
    if (index.isValid() || m_ctrl_key_pressed)
    {
        qDebug() <<"DesktopIconView index copyAction:";
        auto urls = event->mimeData()->urls();
        QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QStringList uris;
        for (auto url : urls)
        {
            if (url.toString() == "computer:///")
                uris << "computer:///";
            else
                uris << url.path();
        }

        //fix can drag copy home folder issue, link to bug#64824
        if (uris.contains(homePath) || uris.contains("computer:///"))
            return;
    }

    if (!m_ctrl_key_pressed) {
        bool bmoved = false;
        if (index.isValid()) {
            auto uri = m_proxy_model->mapToSource(index).data(Qt::UserRole).toString();
            auto info = FileInfo::fromUri(uri);
            if (!info->isDir()||event->mimeData()->urls().contains(uri)) {
                m_pressedPos = QPoint(-1,-1);
                return;
            }
            bmoved = true;
        }
        bool sucess = true;
        if (bmoved) {
            //move file to desktop folder
            qDebug() << "DesktopIconView move file to folder";
            for (auto uuri : event->mimeData()->urls()) {
                if ("trash:///" == uuri.toDisplayString() || "computer:///" == uuri.toDisplayString()) {
                    m_pressedPos = QPoint(-1,-1);
                    return;
                }
            }

            sucess = m_model->dropMimeData(event->mimeData(), action, -1, -1, this->indexAt(event->pos()));
        } /*else {*/
            // do not trigger file operation, link to: #66345
//            m_model->setAcceptDropAction(false);
//            QAbstractItemView::dropEvent(event);
//            m_model->setAcceptDropAction(true);
//        }
        if (!sucess) {
            m_pressedPos = QPoint(-1,-1);
            return;
        }

        if (m_proxy_model->getDesktopUseAutoLayout()) {
            bool bDropToOtherScreen = false;
            auto selectedItems = selectedIndexes();
            if (view) {
                selectedItems = selectedIndexes();
                if (this != event->source()) {
                    bDropToOtherScreen = true;
                    selectedItems = view->selectedIndexes();
                }
                auto destSortedUris = m_autoArrange;
                auto sourceSortedUris = view->m_autoArrange;
                for (auto index : selectedItems) {
                    auto uri = index.data(Qt::UserRole).toString();
                    if (bDropToOtherScreen) {
                        sourceSortedUris.removeOne(uri);
                    } else {
                        destSortedUris.removeOne(uri);
                    }
                }
                relayoutExsitingItemsAndUpdate(destSortedUris);
                if (bDropToOtherScreen) {
                    view->relayoutExsitingItemsAndUpdate(sourceSortedUris);
                }
                m_pressedPos = QPoint(-1,-1);
                view->m_pressedPos = QPoint(-1,-1);
                return;
            }
        }
    }
    if (event->source() == this && !m_ctrl_key_pressed) {
        qDebug() <<"DesktopIconView index:" <<index <<index.isValid();
        // 计算偏移量，并且设置item data
        //QModelIndex pressedIndex = indexAt(m_pressedPos);
        QPoint pressedGridPos = getGridPosFromMousePos(m_pressedPos);
        QPoint dropedGridPos = getGridPosFromMousePos(pos);

        if (pressedGridPos == dropedGridPos) {
            event->ignore();
            m_pressedPos = QPoint(-1,-1);
            return;
        }

        auto gridOffset = dropedGridPos - pressedGridPos;
        qDebug()<<gridOffset;

        QHash<QString, QModelIndex> tmpIndexes;
        QHash<QString, QModelIndex> relayoutIndexes;
        for (QModelIndex index : selectedIndexes()) {
            QString uri = index.data(UriRole).toString();
            tmpIndexes.insert(uri, index);
            bool ok = false;
            auto pos = getIndexGridPos(index, &ok);
            if (ok && !isInvalidPoint(pos)) {
                pos += gridOffset;
                if (isInvalidPoint(pos)|| !m_itemPosHash.keys(pos).isEmpty()) {
                    relayoutIndexes.insert(uri, index);
                    continue;
                }
            } else {
                relayoutIndexes.insert(uri, index);
                continue;
            }

            qDebug()<<" ----------- "<<m_itemPosHash;
            model()->setData(index, pos, PositionRole);
            setFileMetaInfoPos(uri, pos);
        }

        for (QString uri : relayoutIndexes.keys()) {
            QPoint pos = findNextEmptyGridPos(dropedGridPos);
            if (!isInvalidPoint(pos)) {
                qDebug()<<" ----------- "<<m_itemPosHash;
                auto index = relayoutIndexes.value(uri);
                model()->setData(index, pos, PositionRole);
                setFileMetaInfoPos(uri, pos);
            }
        }

        event->ignore();
        m_pressedPos = QPoint(-1,-1);
        return;
    } else if (!m_ctrl_key_pressed && dragToOtherScreen(event)) {
        event->ignore();
        m_pressedPos = QPoint(-1,-1);
        return;
    } else if (!index.isValid() && (event->proposedAction() == Qt::CopyAction  || event->proposedAction() & Qt::MoveAction && m_ctrl_key_pressed)) {
        // fixme: 拷贝
        m_model->dropMimeData(event->mimeData(), action, -1, -1, this->indexAt(event->pos()));
        m_pressedPos = QPoint(-1,-1);
        return;
    }

    return QAbstractItemView::dropEvent(event);
}

bool AdvancedDesktopIconView::dropWhenAotoArrange(QDropEvent *event)
{
    qDebug() << "[AdvancedDesktopIconView::dropWhenAotoArrange]" <<m_itemPosHash << m_autoArrange;

    bool bDropToOtherScreen = false;
    auto selectedItems = selectedIndexes();
    auto view = qobject_cast<AdvancedDesktopIconView*>(event->source());
    if (!view) {
        return false;
    }
    if (this != event->source() && view) {
        bDropToOtherScreen = true;
        selectedItems = view->selectedIndexes();
    }
    if (m_dropIndicatorRect.isValid()) {
        auto currentHoverIndex = indexAt(m_dropIndicatorRect.center() + (m_dropIndicatorPos == DropIndicatorPosition::BelowItem? QPoint(0, -m_gridSize.height()/2): QPoint()));
        if (currentHoverIndex.isValid()) {
            auto currentHoverUri = currentHoverIndex.data(Qt::UserRole).toString();
            auto destSortedUris = m_autoArrange;
            auto sourceSortedUris = view->m_autoArrange;
            QStringList draggedUris;
            for (int i = 0; i < destSortedUris.count(); i++) {
                draggedUris.append(QString());
            }
            int insertedPos = destSortedUris.indexOf(currentHoverUri);
            if (m_dropIndicatorPos == DropIndicatorPosition::BelowItem) {
                insertedPos++;
            }

            if (bDropToOtherScreen) {
                for (auto index : selectedItems) {
                    auto uri = index.data(Qt::UserRole).toString();
                    sourceSortedUris.removeOne(uri);
                    view->model()->setData(index, m_id, ScreenIdRole);
                    destSortedUris.insert(insertedPos, uri);
                }
            } else {
                for (auto index : selectedItems) {
                    auto uri = index.data(Qt::UserRole).toString();
                    int pos = destSortedUris.indexOf(uri);
                    destSortedUris.replace(pos, QString());
                    draggedUris.replace(pos, uri);
                }

                draggedUris.removeAll(QString());

                //draggedUris.reserve(draggedUris.count());
                for (int i = draggedUris.count() - 1; i >= 0; i--) {
                    auto uri = draggedUris[i];
                    destSortedUris.insert(insertedPos, uri);
                }
                destSortedUris.removeAll(QString());
            }

            relayoutExsitingItemsAndUpdate(destSortedUris);
            if (bDropToOtherScreen) {
                view->relayoutExsitingItemsAndUpdate(sourceSortedUris);
            }
            m_proxy_model->setSortedUris(destSortedUris);
            return true;
        } else {
            qCritical()<<"has drop indicator rect but could not find current hovered index";
        }
    } else {
        bool dropToEnd = true;
        auto posIndicator = event->pos();
        for (auto pos : m_itemPosHash.values()) {
            QRect rect(QPoint(pos.x() * m_gridSize.width(), pos.y() * m_gridSize.height()), m_gridSize);
            if (gridPosLesserThan(posIndicator, rect.center())) {
                dropToEnd = false;
                break;
            }
        }
        if (dropToEnd) {
            auto sortedUris = m_autoArrange;
            QStringList draggedUris;
            for (int i = 0; i < sortedUris.count(); i++) {
                draggedUris.append(QString());
            }
            if (!bDropToOtherScreen) {
                for (auto index : selectedItems) {
                    auto uri = index.data(Qt::UserRole).toString();
                    int pos = sortedUris.indexOf(uri);
                    sortedUris.replace(pos, QString());
                    draggedUris.replace(pos, uri);
                }
                sortedUris.removeAll(QString());
                draggedUris.removeAll(QString());
                sortedUris.append(draggedUris);
            } else {
                auto destSortedUris = view->m_autoArrange;
                for (auto index : selectedItems) {
                    auto uri = index.data(Qt::UserRole).toString();
                    destSortedUris.removeOne(uri);
                    draggedUris.append(uri);
                    view->model()->setData(index, m_id, ScreenIdRole);
                }
                sortedUris.removeAll(QString());
                draggedUris.removeAll(QString());
                sortedUris.append(draggedUris);
                view->relayoutExsitingItemsAndUpdate(destSortedUris);
            }

            relayoutExsitingItemsAndUpdate(sortedUris);
            return true;
        } else {
            qDebug()<<"do nothing for drop auto layout";
            if (m_dropIndicatorPos != OnItem) {
                // 如果不是移动拖拽到图标上，则不触发后续拖拽流程，避免出现图标未自动排列的情况
                return true;
            }
        }
    }
    return false;
}

void AdvancedDesktopIconView::wheelEvent(QWheelEvent *e)
{
    if (QApplication::keyboardModifiers() == Qt::ControlModifier)
    {
        if (e->delta() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
    }
}

void AdvancedDesktopIconView::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Home: {
        QPoint homePos;
        QModelIndex homeIndex;
        for (int i = 0; i < m_proxy_model->rowCount(); i++) {
            auto index = m_proxy_model->index(i, 0);
            QPoint postion = index.data(PositionRole).toPoint();
            if (!gridPosLesserThan(homePos, postion)) {
                homeIndex = index;
                homePos = index.data(PositionRole).toPoint();
            }
        }
        selectionModel()->select(homeIndex, QItemSelectionModel::SelectCurrent);
        break;
    }
    case Qt::Key_End: {
        QPoint endPos;
        QModelIndex endIndex;
        for (int i = 0; i < m_proxy_model->rowCount(); i++) {
            auto index = m_proxy_model->index(i, 0);
            QPoint postion = index.data(PositionRole).toPoint();
            if (gridPosLesserThan(endPos, postion)) {
                endIndex = index;
                endPos = postion;
            }
        }
        selectionModel()->select(endIndex, QItemSelectionModel::SelectCurrent);
        break;
    }
    case Qt::Key_Up: {
        if (getSelections().isEmpty()) {
            selectionModel()->select(model()->index(0, 0), QItemSelectionModel::SelectCurrent);
        } else {
            auto index = selectionModel()->selectedIndexes().first();
            auto center = visualRect(index).center();
            auto up = center - QPoint(0, m_gridSize.height());
            auto upIndex = indexAt(up);
            if (upIndex.isValid()) {
                clearAllIndexWidgets();
                selectionModel()->select(upIndex, QItemSelectionModel::SelectCurrent);
                auto delegate = qobject_cast<DesktopIconViewDelegate *>(itemDelegate());
                auto indexWidget = new DesktopIndexWidget(delegate, viewOptions(), upIndex, this);
                setIndexWidget(upIndex, indexWidget);
                indexWidget->move(visualRect(upIndex).topLeft());
            }
        }
        return;
    }
    case Qt::Key_Down: {
        if (getSelections().isEmpty()) {
            selectionModel()->select(model()->index(0, 0), QItemSelectionModel::SelectCurrent);
        } else {
            auto index = selectionModel()->selectedIndexes().first();
            auto center = visualRect(index).center();
            auto down = center + QPoint(0, m_gridSize.height());
            auto downIndex = indexAt(down);
            if (downIndex.isValid()) {
                clearAllIndexWidgets();
                selectionModel()->select(downIndex, QItemSelectionModel::SelectCurrent);
                auto delegate = qobject_cast<DesktopIconViewDelegate *>(itemDelegate());
                auto indexWidget = new DesktopIndexWidget(delegate, viewOptions(), downIndex, this);
                setIndexWidget(downIndex, indexWidget);
                indexWidget->move(visualRect(downIndex).topLeft());
            }
        }
        return;
    }
    case Qt::Key_Left: {
        if (getSelections().isEmpty()) {
            selectionModel()->select(model()->index(0, 0), QItemSelectionModel::SelectCurrent);
        } else {
            auto index = selectionModel()->selectedIndexes().first();
            auto center = visualRect(index).center();
            auto left = center - QPoint(m_gridSize.width(), 0);
            auto leftIndex = indexAt(left);
            if (leftIndex.isValid()) {
                clearAllIndexWidgets();
                selectionModel()->select(leftIndex, QItemSelectionModel::SelectCurrent);
                auto delegate = qobject_cast<DesktopIconViewDelegate *>(itemDelegate());
                auto indexWidget = new DesktopIndexWidget(delegate, viewOptions(), leftIndex, this);
                setIndexWidget(leftIndex, indexWidget);
                indexWidget->move(visualRect(leftIndex).topLeft());
            }
        }
        return;
    }
    case Qt::Key_Right: {
        if (getSelections().isEmpty()) {
            selectionModel()->select(model()->index(0, 0), QItemSelectionModel::SelectCurrent);
        } else {
            auto index = selectionModel()->selectedIndexes().first();
            auto center = visualRect(index).center();
            auto right = center + QPoint(m_gridSize.width(), 0);
            auto rightIndex = indexAt(right);
            if (rightIndex.isValid()) {
                clearAllIndexWidgets();
                selectionModel()->select(rightIndex, QItemSelectionModel::SelectCurrent);
                auto delegate = qobject_cast<DesktopIconViewDelegate *>(itemDelegate());
                auto indexWidget = new DesktopIndexWidget(delegate, viewOptions(), rightIndex, this);
                setIndexWidget(rightIndex, indexWidget);
            }
        }
        return;
    }
    case Qt::Key_Shift:
        m_shift_key_pressed = true;
    case Qt::Key_Control:
        m_ctrl_or_shift_pressed = true;
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
    {
        auto selections = this->getSelections();
        for (auto uri : selections)
        {
           openFileByUri(uri);
        }
    }
        break;
    default:
        return QAbstractItemView::keyPressEvent(e);
    }
}

void AdvancedDesktopIconView::keyReleaseEvent(QKeyEvent *e)
{
    QAbstractItemView::keyReleaseEvent(e);
    m_ctrl_or_shift_pressed = false;
    m_ctrl_key_pressed = false;
    m_shift_key_pressed = false;
}

void AdvancedDesktopIconView::recalculateAvailableRowAndColumnCount()
{
    m_gridSize = QSize(10, 10); // padding: 5
    m_gridSize += iconSize(); // icon size
    m_gridSize += QSize(0, 5); // text-icon-padding: 5
    m_gridSize += QSize(0, qApp->fontMetrics().height()*2); // 2 line text height, todo: fix bo_CN
    m_gridSize.setWidth(qMax(qApp->fontMetrics().averageCharWidth()*5 + 10, m_gridSize.width() + 31));

    auto size = viewport()->rect().size();
    m_availableRowCount = qMax(1, size.height()/m_gridSize.height());
    m_availableColumnCount = qMax(1, size.width()/m_gridSize.width());

    reset();

    resolutionChange();

    checkItemsOver();

    viewport()->update(); //update all item
}

QPoint AdvancedDesktopIconView::getIndexGridPos(const QModelIndex &index, bool *ok) const
{
    //fix bug#249915, unzip file to desktop crash issue
    if (! index.isValid() || ! index.data().isValid()) {
        return invalidPoint();
    }

    *ok = index.data(PositionRole).isValid();
    if (!(*ok))
        return invalidPoint();
    return index.data(PositionRole).toPoint();
}

QPoint AdvancedDesktopIconView::findNextEmptyGridPos(const QPoint &startGridPos)
{
    for (int column = 0; column < m_availableColumnCount; column++) {
        for (int row = 0; row < m_availableRowCount; row++) {
            QPoint pos = QPoint(column, row);
            if (gridPosLesserThan(pos, startGridPos))
                continue;
            if (m_itemPosHash.keys(pos).isEmpty())
                return pos;
        }
    }
    return invalidPoint();
}

bool AdvancedDesktopIconView::isInvalidPoint(const QPoint &point)
{
    return point.x() < 0 || point.x() >= m_availableColumnCount || point.y() < 0 || point.y() >= m_availableRowCount;
}

QPoint AdvancedDesktopIconView::invalidPoint() const
{
    return QPoint(-1, -1);
}

QPoint AdvancedDesktopIconView::getGridPosFromMousePos(const QPoint &mousePos)
{
    int x = mousePos.x()/m_gridSize.width();
    if (layoutDirection() == Qt::RightToLeft) {
        int vewportX = viewport()->rect().topRight().x();
        x = (vewportX - mousePos.x())/m_gridSize.width();
    }
    return QPoint(x, mousePos.y()/m_gridSize.height());
}

bool AdvancedDesktopIconView::gridPosLesserThan(const QPoint &leftPos, const QPoint &rightPos)
{
    if (leftPos.x() < rightPos.x())
        return true;

    if (leftPos.x() > rightPos.x())
        return false;

    if (leftPos.y() < rightPos.y())
        return true;
    return false;
}
const QStringList AdvancedDesktopIconView::getSelections()
{
    QStringList uris;
    auto indexes = selectionModel()->selection().indexes();
    for (auto index : indexes) {
        uris<<index.data(Qt::UserRole).toString();
    }
    uris.removeDuplicates();
    return uris;
}

const QStringList AdvancedDesktopIconView::getAllFileUris()
{
    QStringList uris;
    for (int i = 0; i < m_proxy_model->rowCount(); i++) {
        auto index = m_proxy_model->index(i, 0);
        uris<<index.data(Qt::UserRole).toString();
    }
    return uris;
}

const int AdvancedDesktopIconView::getAllDisplayFileCount()
{
    if(m_proxy_model)
        return m_proxy_model->rowCount();
    return 0;
}

void AdvancedDesktopIconView::setSelections(const QStringList &uris)
{
    clearSelection();
    for (int i = 0; i < m_proxy_model->rowCount(); i++) {
        auto index = m_proxy_model->index(i, 0);
        if (uris.contains(index.data(Qt::UserRole).toString())) {
            selectionModel()->select(index, QItemSelectionModel::Select);
        }
    }
}

void AdvancedDesktopIconView::invertSelections()
{
    QItemSelectionModel *selectionModel = this->selectionModel();
    const QItemSelection currentSelection = selectionModel->selection();
    this->selectAll();
    selectionModel->select(currentSelection, QItemSelectionModel::Deselect);
    //clearAllIndexWidgets();
}

void AdvancedDesktopIconView::setEditFlag(bool edit)
{
    qDebug() << "setEditFlag:" <<edit;
    m_is_edit = edit;
    if (! m_is_edit)
        m_edit_uri = "";
}

bool AdvancedDesktopIconView::getEditFlag()
{
    return m_is_edit;
}

bool AdvancedDesktopIconView::isRenaming()
{
    return m_is_renaming;
}

void AdvancedDesktopIconView::setRenaming(bool renaming)
{
    m_is_renaming = renaming;
}

int AdvancedDesktopIconView::updateBWList()
{
    m_proxy_model->updateBlackAndWriteLists();
    /*
    * 重新按照既定规则排序，这样可以避免出现空缺和图标重叠的情况
    */
    //不可重新排序，会丢失位置，只做检查调整空缺和重叠情况，相关bug#161875
    resolutionChange();
    viewport()->update();
    return 0;
}

QString AdvancedDesktopIconView::getBlackAndWhiteModel()
{
    return m_proxy_model->getBlackAndWhiteModel();
}

QSet<QString> AdvancedDesktopIconView::getBWListInfo()
{
    return m_proxy_model->getBWListInfo();
}

bool AdvancedDesktopIconView::getBlackAndWhiteListExist(QString name)
{
   return m_proxy_model->getBlackAndWhiteListExist(name);
}

void AdvancedDesktopIconView::setSortType(int sortType)
{
    m_resolutionItemPosHash.clear();
    m_itemPosHash.clear();
    m_autoArrange.clear();
    m_storageBox.clear();
    m_proxy_model->setSortType(sortType);
    m_proxy_model->sort(1);
    m_proxy_model->sort(0, Qt::SortOrder(m_proxy_model->getSortOrder()));
    for (int i = 0; i < m_proxy_model->rowCount(); i++) {
        auto index = m_proxy_model->index(i, 0);
        QString uri = index.data(Qt::UserRole).toString();

        int col = i/m_availableRowCount;
        int row = i%m_availableRowCount;
        if (col >= m_availableColumnCount) {
            row = 0;
            col = 0;
            m_storageBox.append(uri);
        }
        QPoint pos(col, row);
        model()->setData(index, pos, PositionRole);
        setFileMetaInfoPos(uri, pos);
        m_autoArrange.append(uri);
    }
}

int AdvancedDesktopIconView::getSortType()
{
    return m_proxy_model->getSortType();
}

void AdvancedDesktopIconView::scrollToSelection(const QString &uri)
{

}

int AdvancedDesktopIconView::getSortOrder()
{
    return m_proxy_model->getSortOrder();
}

void AdvancedDesktopIconView::setSortOrder(int sortOrder)
{
    m_resolutionItemPosHash.clear();
    m_itemPosHash.clear();
    m_autoArrange.clear();
    m_storageBox.clear();
    m_proxy_model->setSortOrder(sortOrder);
    m_proxy_model->sort(0, Qt::SortOrder(sortOrder));
    for (int i = 0; i < m_proxy_model->rowCount(); i++) {
        auto index = m_proxy_model->index(i, 0);
        QString uri = index.data(Qt::UserRole).toString();

        int col = i/m_availableRowCount;
        int row = i%m_availableRowCount;
        if (col >= m_availableColumnCount) {
            row = 0;
            col = 0;
            m_storageBox.append(uri);
        }
        QPoint pos(col, row);
        model()->setData(index, pos, PositionRole);
        setFileMetaInfoPos(uri, pos);
        m_autoArrange.append(uri);
    }
}

void AdvancedDesktopIconView::editUri(const QString &uri)
{
    clearAllIndexWidgets();
    qDebug() << "editUri clearAllIndexWidgets";
    auto origin = FileUtils::getOriginalUri(uri);
    QTimer::singleShot(100, this, [=]() {
        auto index = m_proxy_model->mapFromSource(m_model->indexFromUri(origin));
        edit(index);
        qDebug() << "editUri index:"<<index<<uri;
    });
}

void AdvancedDesktopIconView::editUris(const QStringList uris)
{
    clearAllIndexWidgets();
    auto origin = FileUtils::getOriginalUri(uris.first());
    QTimer::singleShot(100, this, [=]() {
        edit(m_proxy_model->mapFromSource(m_model->indexFromUri(origin)));
    });
}


void AdvancedDesktopIconView::UpdateToEditUris(QStringList uris)
{
    m_uris_to_edit = uris;
}

void AdvancedDesktopIconView::setCutFiles(const QStringList &uris)
{
    ClipboardUtils::setClipboardFiles(uris, true);
    this->viewport()->update();
}

void AdvancedDesktopIconView::closeView()
{
    deleteLater();
}
void AdvancedDesktopIconView::fileCreated(const QString &uri)
{
//    qDebug()<<"DesktopIconView::fileCreated,view:" << this <<m_new_files_to_be_selected.length();
//    if (m_new_files_to_be_selected.isEmpty()) {
//        m_new_files_to_be_selected<<uri;

//        QTimer::singleShot(500, this, [=]() {
//            qDebug() << "m_new_files_to_be_selected isEmpty:"<<this->state();
//            if (this->state() & QAbstractItemView::EditingState)
//                return;

//            if (! this->m_uris_to_edit.isEmpty())
//                return;
//            qDebug() << "fileCreated setSelections"<<m_new_files_to_be_selected.length();
//            this->setSelections(m_new_files_to_be_selected);
//            m_new_files_to_be_selected.clear();
//        });
//    } else {
//        if (!m_new_files_to_be_selected.contains(uri)) {
//            m_new_files_to_be_selected<<uri;
//        }
//    }

    if (m_proxy_model->getDesktopUseAutoLayout()) {
        //自动排序情况下如果有重复名字导致替换后出现空位，需要重新排序
        clearAllIndexWidgets();
        bool haveEmptyPosition = checkEmptyPositon();
        if (haveEmptyPosition) {
            m_autoArrange = layoutItems();
        }
    }
    /* 新建文件/文件夹，可编辑文件名，copy时不能编辑 */
    //fix bug#164160, use same way as mainwindow
    if(this->m_uris_to_edit.isEmpty())
        return;

    QString editUri = Peony::FileUtils::urlDecode(this->m_uris_to_edit.first());
    QString infoUri = Peony::FileUtils::urlDecode(uri);
    qDebug() << "fileCreated editUri:"<<editUri<<infoUri;
    if (editUri == infoUri ) {
        QTimer::singleShot(100, this, [=]() {
            this->editUri(uri);
            m_edit_uri = uri;
        });
    }
    this->m_uris_to_edit.clear();
}

bool AdvancedDesktopIconView::dragToOtherScreen(QDropEvent *event)
{
    // 从其它视图拖拽到此视图空白区域
    auto view = qobject_cast<AdvancedDesktopIconView*>(event->source());
    if (this != event->source() && view) {
        QPoint pressedGridPos = view->getGridPosFromMousePos(view->m_pressedPos);
        QPoint dropedGridPos = getGridPosFromMousePos(event->pos());
        m_pressedPos = QPoint(-1,-1);
        view->m_pressedPos = QPoint(-1,-1);
        m_rubberBand->setVisible(false);
        auto gridOffset = dropedGridPos - pressedGridPos;
        qDebug()<<gridOffset;

        QHash<QString, QModelIndex> relayoutIndexes;
        QModelIndexList dragIndex = view->selectedIndexes();
        for (QModelIndex index : dragIndex) {
            QString uri = index.data(UriRole).toString();
            bool ok = false;
            auto pos = getIndexGridPos(index, &ok);
            if (ok && !isInvalidPoint(pos)) {
                pos += gridOffset;
                if (isInvalidPoint(pos) || !m_itemPosHash.keys(pos).isEmpty()) {
                    relayoutIndexes.insert(uri, index);
                    continue;
                }
            } else {
                relayoutIndexes.insert(uri, index);
                continue;
            }
            m_itemPosHash.insert(uri, pos);
            view->m_itemPosHash.remove(uri);
            qDebug()<<" ----------- "<<m_itemPosHash << index << uri << "============" <<index;
            view->model()->setData(index, pos, PositionRole);
            view->model()->setData(index, m_id, ScreenIdRole);
            setFileMetaInfoPos(uri, pos);
        }

        for (QString uri : relayoutIndexes.keys()) {
            QPoint pos = findNextEmptyGridPos(dropedGridPos);
            if (!isInvalidPoint(pos)) {
                m_itemPosHash.insert(uri, pos);
                qDebug()<<" ----------- "<<m_itemPosHash;
                auto index = relayoutIndexes.value(uri);
                view->model()->setData(index, pos, PositionRole);
                view->model()->setData(index, m_id, ScreenIdRole);
                qDebug()<<" -----------relayoutIndexes "<<m_itemPosHash << index << uri;
                setFileMetaInfoPos(uri, pos);
            }
        }
        m_proxy_model->invalidate();
        view->m_proxy_model->invalidate();
        return true;
    }
    return false;
}

void AdvancedDesktopIconView::mousePressEvent(QMouseEvent *e)
{
    m_dragPressPos = e->pos();
    if (e->button() == Qt::LeftButton) {
        m_pressedPos = e->pos();
        m_rubberBand->setGeometry(QRect(e->pos(), QSize()));
        m_rubberBand->setVisible(true);
    } else {
        m_pressedPos = QPoint(-1,-1);
    }

    // bug extend selection bug
    m_real_do_edit = false;

    if (e->modifiers() & Qt::ShiftModifier) {
        m_shift_key_pressed = true;
    } else {
        m_shift_key_pressed = false;
    }

    if (e->modifiers() & Qt::ControlModifier)
        m_ctrl_key_pressed = true;
    else {
        m_ctrl_key_pressed = false;
        if (!m_shift_key_pressed)
            m_ctrl_or_shift_pressed = false;
    }

    auto index = indexAt(e->pos());
    if (!m_ctrl_or_shift_pressed) {
        if (!index.isValid()) {
            clearAllIndexWidgets();
            clearSelection();
            Q_EMIT clearOtherViewSelection();
        } else {
            m_last_index = index;
            //fix rename state has no menuRequest issue, bug#44107
            if (! m_is_edit)
            {
                clearAllIndexWidgets();
                Q_EMIT clearOtherViewSelection();
                //force to recreate new DesktopIndexWidget, to fix not show name issue
                if (indexWidget(m_last_index))
                    setIndexWidget(m_last_index, nullptr);
                auto indexWidget = new DesktopIndexWidget(qobject_cast<DesktopIconViewDelegate *>(itemDelegate()), viewOptions(), m_last_index);
                setIndexWidget(m_last_index,
                               indexWidget);
                indexWidget->move(visualRect(m_last_index).topLeft());
            }
        }
    }

    //qDebug()<<m_last_index.data();
    if (e->button() != Qt::LeftButton) {
        // fix #115384, context menu key issue
        if (e->button() == Qt::RightButton) {
            auto index = indexAt(e->pos());
            if (!selectedIndexes().contains(index)) {
                selectionModel()->select(index, QItemSelectionModel::SelectCurrent);
            }
        }
        return;
    }

    if (e->button() == Qt::LeftButton && e->modifiers() & Qt::ControlModifier && selectedIndexes().contains(index)) {
        m_noSelectOnPress = true;
    } else {
        m_noSelectOnPress = false;
    }

    QAbstractItemView::mousePressEvent(e);
}

void AdvancedDesktopIconView::mouseReleaseEvent(QMouseEvent *event)
{
    m_pressedPos = QPoint(-1, -1);
    m_rubberBand->setVisible(false);
    m_noSelectOnPress = false;
    QAbstractItemView::mouseReleaseEvent(event);
    this->viewport()->update(viewport()->rect());
}

void AdvancedDesktopIconView::mouseMoveEvent(QMouseEvent *event)
{
    QAbstractItemView::mouseMoveEvent(event);

    QModelIndex itemIndex = indexAt(event->pos());
    if (!itemIndex.isValid()) {
        if (QToolTip::isVisible()) {
            QToolTip::hideText();
        }
    }

    auto tmpRect = QRect(event->pos(), m_pressedPos).normalized();
    if (m_pressedPos.x() >= 0 &&  m_pressedPos.y() >= 0 && m_rubberBand->isVisible() && state() != QAbstractItemView::DraggingState) {
        tmpRect.translate(viewportMargins().left(), viewportMargins().top());
        m_rubberBand->setGeometry(tmpRect);
        setState(QAbstractItemView::DragSelectingState);
    }

    // ugly, 需要优化
    if (state() == QAbstractItemView::DragSelectingState)
        setSelection(tmpRect, selectionCommand(indexAt(event->pos()), event));
}

void AdvancedDesktopIconView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (! GlobalSettings::getInstance()->getValue(ENABLE_DOUBLE_CLICK_DESKTOP).toBool())
        return;

    QAbstractItemView::mouseDoubleClickEvent(event);
    m_real_do_edit = false;
}

void AdvancedDesktopIconView::dragEnterEvent(QDragEnterEvent *e)
{
    m_real_do_edit = false;
    m_isDragging = true;
    auto action = m_ctrl_key_pressed ? Qt::CopyAction : Qt::MoveAction;
    qDebug()<<"drag enter event" <<action;
    if (e->mimeData()->hasUrls()) {
        if (FileUtils::containsStandardPath(e->mimeData()->urls())) {
            e->ignore();
            return;
        }
        e->setDropAction(action);
        e->accept();
        //e->acceptProposedAction();
    }
}

void AdvancedDesktopIconView::dragMoveEvent(QDragMoveEvent *e)
{
    m_real_do_edit = false;
    if (e->keyboardModifiers() & Qt::ControlModifier)
        m_ctrl_key_pressed = true;
    else
        m_ctrl_key_pressed = false;

    auto action = m_ctrl_key_pressed ? Qt::CopyAction : Qt::MoveAction;
    auto index = indexAt(e->pos());
    if (index.isValid() && index != m_last_index) {
        QHoverEvent he(QHoverEvent::HoverMove, e->posF(), e->posF());
        viewportEvent(&he);
    } else {
        QHoverEvent he(QHoverEvent::HoverLeave, e->posF(), e->posF());
        viewportEvent(&he);
    }
    if (/*e->source() == this &&*/ m_proxy_model->getDesktopUseAutoLayout()) {
        if (index.isValid()) {
            auto oldRect = m_dropIndicatorRect;
            auto rect = visualRect(index);
            int y = (m_gridSize.height() - rect.height())/2 + 1;
            if (e->pos().y() - rect.top() <= 5) {
                m_dropIndicatorRect = rect;
                m_dropIndicatorRect.setBottom(rect.top() + 1);
                m_dropIndicatorRect.translate(0, -y);
                m_dropIndicatorPos = DropIndicatorPosition::AboveItem;
            } else if (rect.bottom() - e->pos().y() <= 20) {
                m_dropIndicatorRect = rect;
                m_dropIndicatorRect.setTop(rect.bottom() - 1);
                m_dropIndicatorRect.translate(0, y);
                m_dropIndicatorPos = DropIndicatorPosition::BelowItem;
            } else {
                m_dropIndicatorRect = QRect();
                m_dropIndicatorPos = DropIndicatorPosition::OnItem;
            }
            if (oldRect != m_dropIndicatorRect)
                viewport()->update(rect.adjusted(0, -100, 0, 100));
        } else {
            if (m_dropIndicatorRect.isValid()) {
                viewport()->update(m_dropIndicatorRect.adjusted(0, -100, 0, 100));
            }
            m_dropIndicatorRect = QRect();
            m_dropIndicatorPos = DropIndicatorPosition::OnViewport;
        }
    }
    e->setDropAction(action);
    if (e->isAccepted())
        return;
    qDebug()<<"drag move event" <<action;
    if (this == e->source()) {
        e->accept();
        return QAbstractItemView::dragMoveEvent(e);
    }
    e->accept();
}

void AdvancedDesktopIconView::dragLeaveEvent(QDragLeaveEvent *e)
{
    m_isDragging = false;
    QAbstractItemView::dragLeaveEvent(e);
}

bool AdvancedDesktopIconView::viewportEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::HoverMove:
    case QEvent::HoverEnter:
        onEntered(indexAt(static_cast<QHoverEvent*>(e)->pos()));
        break;
    case QEvent::HoverLeave:
        onEntered(QModelIndex());
        break;
    case QEvent::Leave:
        onEntered(QModelIndex()); // If we've left, no hover should be needed anymore
        m_rubberBand->setVisible(false);
        break;
    default:
        break;
    }
    return QAbstractItemView::viewportEvent(e);
}

QItemSelectionModel::SelectionFlags AdvancedDesktopIconView::selectionCommand(const QModelIndex &index, const QEvent *event) const
{
    if (!event)
        return QAbstractItemView::selectionCommand(index, event);

    if (event->type() == QEvent::MouseButtonPress) {
        auto e = static_cast<const QMouseEvent *>(event);
        if (e->button() == Qt::LeftButton && e->modifiers() & Qt::ControlModifier && selectedIndexes().contains(index)) {
            return QItemSelectionModel::NoUpdate;
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        auto e = static_cast<const QMouseEvent *>(event);
        if (e->button() == Qt::LeftButton && e->modifiers() & Qt::ControlModifier) {
            QItemSelectionModel::SelectionFlags flags;
            if (m_noSelectOnPress) {
                flags = QItemSelectionModel::Deselect;
            }
            return flags;
        }
    }
    return QAbstractItemView::selectionCommand(index, event);
}

void AdvancedDesktopIconView::zoomOut()
{
    clearAllIndexWidgets();
    switch (zoomLevel()) {
    case Huge:
        Q_EMIT setZoomLevel(Large);
        break;
    case Large:
        Q_EMIT setZoomLevel(Normal);
        break;
    case Normal:
        Q_EMIT setZoomLevel(Small);
        break;
    default:
        //setDefaultZoomLevel(zoomLevel());
        break;
    }
}

void AdvancedDesktopIconView::zoomIn()
{
    clearAllIndexWidgets();
    switch (zoomLevel()) {
    case Small:
        Q_EMIT setZoomLevel(Normal);
        break;
    case Normal:
        Q_EMIT setZoomLevel(Large);
        break;
    case Large:
        Q_EMIT setZoomLevel(Huge);
        break;
    default:
        //setDefaultZoomLevel(zoomLevel());
        break;
    }
}

void AdvancedDesktopIconView::setDefaultZoomLevel(ZoomLevel level)
{
    //qDebug()<<"set default zoom level:"<<level;
    m_zoom_level = level;
    m_margin = QPoint(10, 5);
    switch (level) {
    case Small:
        m_margin *= 0.8;
        setIconSize(QSize(24, 24));
        break;
    case Large:
        m_margin *= 1.2;
        setIconSize(QSize(64, 64));
        break;
    case Huge:
        m_margin *= 1.4;
        setIconSize(QSize(96, 96));
        break;
    default:
        m_zoom_level = Normal;
        setIconSize(QSize(48, 48));
        break;
    }

    auto metaInfo = FileMetaInfo::fromUri("computer:///");
    if (metaInfo) {
        qDebug()<<"set zoom level"<<m_zoom_level;
        metaInfo->setMetaInfoInt("peony-qt-desktop-zoom-level", int(m_zoom_level));
    }
}

AdvancedDesktopIconView::ZoomLevel AdvancedDesktopIconView::zoomLevel() const
{
    //FIXME:
    if (m_zoom_level != Invalid)
        return m_zoom_level;

    auto metaInfo = FileMetaInfo::fromUri("computer:///");
    if (metaInfo) {
        auto i = metaInfo->getMetaInfoInt("peony-qt-desktop-zoom-level");
        return ZoomLevel(i);
    }

    GFile *computer = g_file_new_for_uri("computer:///");
    g_file_query_info_async(computer,
                            "metadata::peony-qt-desktop-zoom-level",
                            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                            G_PRIORITY_DEFAULT,
                            nullptr,
                            GAsyncReadyCallback(queryZoomLevelAsyncCallback),
                            const_cast<AdvancedDesktopIconView *>(this));
    g_object_unref(computer);
    return Normal;
}

void AdvancedDesktopIconView::clearAllIndexWidgets(const QStringList &uris)
{
    if (!model())
        return;
    m_hoverIndex = QModelIndex();
    //fix bug#164160, when edit new file, infoUpdate call clearAllIndexWidgets issue
    if (m_is_edit && uris.length()>0 && m_edit_uri == uris.first())
        return;

    if(uris.length()>0 )
       qDebug() << "clearAllIndexWidgets uris:"<<uris.first()<<uris.length();

    int row = 0;
    auto index = model()->index(row, 0);
    while (index.isValid()) {
        if (uris.isEmpty() || uris.contains(index.data(Qt::UserRole).toString())) {
            auto widget = indexWidget(index);
            if (widget) {
                widget->hide();
            }
            setIndexWidget(index, nullptr);
            //qDebug() << "clearAllIndexWidgets setIndexWidget"<<index;
        }
        row++;
        index = model()->index(row, 0);
    }

    // avoid dirty region out of index visual rect.
    // link to: #77272.
    viewport()->update();
}

void AdvancedDesktopIconView::setFileMetaInfoPos(const QString &uri, const QPoint &pos)
{
    m_itemPosHash.insert(uri, pos);
    QPoint currentPos(pos.x()*m_gridSize.width(), pos.y()*m_gridSize.height());
    QRect rect(mapToGlobal(currentPos)*qApp->devicePixelRatio(), m_gridSize*qApp->devicePixelRatio());
    FileInfo::fromUri(uri).get()->setProperty("iconGeometry", rect);
}

void AdvancedDesktopIconView::resolutionChange()
{
    qInfo()<<"start resolutionChange()";
    QSize screenSize = this->viewport()->size();
    // do not relayout items while screen size is empty.
    if (screenSize.isEmpty()) {
        qWarning()<<"screen size is not available";
        return;
    }

    if (m_itemPosHash.isEmpty()) {
        return;
    }
    if (m_proxy_model && m_proxy_model->getDesktopUseAutoLayout()) {
       // setSortType(int(DesktopItemProxyModel::SortType::RelatedPos));
        relayoutExsitingItemsAndUpdate(m_autoArrange);
        return;
    }
    QStringList needChanged;
    for (QString uri : m_itemPosHash.keys()) {
        QPoint pos = m_itemPosHash.value(uri);
        if (isInvalidPoint(pos)) {
            needChanged.append(uri);
            if (!m_resolutionItemPosHash.contains(uri)) {
                // remember item position before resolution changed.
                m_resolutionItemPosHash.insert(uri, pos);
            }
        }
    }
    qInfo()<<"need changed item"<<needChanged << " resolution item" << m_resolutionItemPosHash;

    if (!needChanged.isEmpty()) {
        qInfo()<<"屏幕过小，有元素超过屏幕范围" ;
        relayoutExsitingItems(needChanged);
    } else {
        qInfo()<<"没有元素超过屏幕范围， 尝试恢复超过屏幕范围的元素" <<m_resolutionItemPosHash;
        QStringList itemNeedBeRelayout;
        for (auto uri : m_resolutionItemPosHash.keys()) {
            auto pos = m_resolutionItemPosHash.value(uri);
            if (isInvalidPoint(pos)) {
                itemNeedBeRelayout.append(uri);
            } else {
                m_resolutionItemPosHash.remove(uri);
                m_itemPosHash.insert(uri, pos);
            }
        }
        relayoutExsitingItems(itemNeedBeRelayout);
    }

    for (int row = 0; row < model()->rowCount(); row++) {
        // todo: 桌面整理时的表现
        auto index = model()->index(row, 0, QModelIndex());
        auto uri = index.data(UriRole).toString();
        auto pos = m_itemPosHash.value(uri);
        model()->setData(index, pos, PositionRole);
        setFileMetaInfoPos(uri, pos);
        if (m_resolutionItemPosHash.contains(uri)) {
            QPoint reolutionItemPos = m_resolutionItemPosHash.value(uri);
            QStringList exceptedPos;
            exceptedPos<<QString::number(reolutionItemPos.x());
            exceptedPos<<QString::number(reolutionItemPos.y());
            exceptedPos<<QString::number(m_id);
            model()->setData(index, exceptedPos, ExceptedPositionRole);
        }
    }
}

void AdvancedDesktopIconView::relayoutExsitingItems(const QStringList &uris)
{
    if (uris.isEmpty() || m_itemPosHash.isEmpty()) {
        return;
    }
    for (auto uri : uris) {
        m_itemPosHash.remove(uri);
    }
    auto allFileUris = getAllFileUris();

    QPoint startPos = QPoint();
    int i = 0;
    for (; i < uris.count(); i++) {
        QString uri = uris[i];
        if (!allFileUris.contains(uri))
            continue;
        startPos = findNextEmptyGridPos(startPos);
        if (isInvalidPoint(startPos)) {
            break;
        }
        setFileMetaInfoPos(uri, startPos);
        m_storageBox.removeOne(uri);
    }

    for(; i < uris.count(); i++) {
        QString uri = uris[i];
        setFileMetaInfoPos(uri, QPoint(0, 0));
        if (!m_storageBox.contains(uri)) {
            m_storageBox.append(uri);
        }
    }
}

static bool iconSizeLessThan (const QPair<QPoint, QString>& p1, const QPair<QPoint, QString>& p2)
{
    if (p1.first.x() > p2.first.x())
        return false;

    if (p1.first.x() < p2.first.x())
        return true;

    if ((p1.first.x() == p2.first.x()))
        return p1.first.y() < p2.first.y();

    return true;
}

QStringList AdvancedDesktopIconView::layoutItems(bool closest)
{
    if (!closest) {
        QHash<QString, QPoint> extendMap;
        QHash<QString, QPoint> primaryItemMap;
        QList<QPair<QPoint, QString>> newPosition;
        QStringList overItem;
        for (int row = 0; row < model()->rowCount(); row++) {
            // todo: 桌面整理时的表现
            auto index = model()->index(row, 0, QModelIndex());
            auto uri = index.data(UriRole).toString();
            QPoint pos = index.data(PositionRole).toPoint();
            auto extendPos = index.data(ExtendScreenPositionRole).toStringList();
            if (3 == extendPos.count() && 0 != extendPos[2].toInt()) {
                extendMap.insert(uri, pos);
            } else {
                newPosition << QPair<QPoint, QString>(pos, uri);
                primaryItemMap.insert(uri, pos);
            }
        }

        for (QString uri : extendMap.keys()) {
            QPoint pos = extendMap.value(uri);
            if (primaryItemMap.keys(pos).isEmpty()) {
                newPosition << QPair<QPoint, QString>(pos, uri);
            } else {
                overItem.append(uri);
            }
        }

        if (!newPosition.isEmpty()) {

            std::stable_sort(newPosition.begin(), newPosition.end(), iconSizeLessThan);

            QStringList sortedUris;
            for (auto pair : newPosition) {
                if (m_storageBox.contains(pair.second)) {
                    overItem.prepend(pair.second);
                } else {
                    sortedUris.append(pair.second);
                }
            }
            sortedUris << overItem;
            sortedUris.removeDuplicates();
            qDebug() <<"--------------------layoutItems" <<sortedUris;
            relayoutExsitingItemsAndUpdate(sortedUris);
            return sortedUris;
        }
    } else {
        qInfo()<<"not supported yet";
    }
    return QStringList();
}

void AdvancedDesktopIconView::relayoutExsitingItemsAndUpdate(const QStringList uris)
{
    if (m_availableColumnCount == 1 && m_availableRowCount == 1)
        return;
    m_itemPosHash.clear();
    m_autoArrange.clear();
    m_storageBox.clear();
    int index = 0;
    //按照行和列顺序放置uri
    for (int column = 0; column < m_availableColumnCount && index < uris.size(); column++) {
        for (int row = 0; row < m_availableRowCount && index < uris.size(); row++) {
            auto uri = uris[index];
            QPoint pos = QPoint(column, row);
            setFileMetaInfoPos(uri, pos);
            m_autoArrange.append(uri);
            index++;
        }
    }

    for (; index < uris.size(); index++) {
        //满屏暂时都放到（0，0）位置
        auto uri = uris[index];
        QPoint pos = QPoint(0, 0);
        setFileMetaInfoPos(uri, pos);
        m_autoArrange.append(uri);
        m_storageBox.append(uri);
    }

    m_autoArrange.removeDuplicates();
    m_proxy_model->invalidate();
    for (int row = 0; row < model()->rowCount(); row++) {
        // todo: 桌面整理时的表现
        auto index = model()->index(row, 0, QModelIndex());
        auto uri = index.data(UriRole).toString();
        if(!uris.contains(uri))
            continue;
        auto pos = m_itemPosHash.value(uri);
        model()->setData(index, pos, PositionRole);
    }
    qDebug() << "AdvancedDesktopIconView::relayoutExsitingItemsAndUpdat---------" <<m_itemPosHash;

    viewport()->update();
}

void AdvancedDesktopIconView::checkItemsOver()
{
    QStringList primaryItem, extendItem;
    QStringList needRelayoutItems;
    for (int i = 0; i < model()->rowCount(); i++) {
        QModelIndex index = model()->index(i, 0);
        QString uri = index.data(Qt::UserRole).toString();
        QPoint pos = index.data(PositionRole).toPoint();
        auto overUris = m_itemPosHash.keys(pos);
        if (overUris.count() > 1) {
            auto extendPos = index.data(ExtendScreenPositionRole).toStringList();
            if (3 == extendPos.count() && 0 != extendPos[2].toInt()) {
                extendItem.append(uri);
            } else {
                primaryItem.append(uri);
            }
        }
    }
    primaryItem<<extendItem;
    for (auto uri : primaryItem) {
        if (needRelayoutItems.contains(uri)) {
            continue;
        }
        auto pos = m_itemPosHash.value(uri);
        auto list = m_itemPosHash.keys(pos);
        list.removeAll(uri);
        needRelayoutItems << list;
    }

    needRelayoutItems.removeDuplicates();
    qDebug() << "checkitemsover" <<needRelayoutItems;

    if (0 == needRelayoutItems.size()) {
        return;
    }

    relayoutExsitingItems(needRelayoutItems);

    for (int row = 0; row < model()->rowCount(); row++) {
        // todo: 桌面整理时的表现
        auto index = model()->index(row, 0, QModelIndex());
        auto uri = index.data(UriRole).toString();
        auto pos = m_itemPosHash.value(uri);
        model()->setData(index, pos, PositionRole);
        setFileMetaInfoPos(uri, pos);
    }
}

GAsyncReadyCallback AdvancedDesktopIconView::queryZoomLevelAsyncCallback(GObject *obj, GAsyncResult *res, AdvancedDesktopIconView *p_this)
{
    GError *err = nullptr;
    auto info = g_file_query_info_finish(G_FILE(obj), res, &err);

    if (info) {
        char* zoom_level = g_file_info_get_attribute_as_string(info, "metadata::peony-qt-desktop-zoom-level");
        if (!zoom_level) {
            //qDebug()<<"======================no zoom level meta info\n\n\n";
            g_object_unref(info);
            p_this->setDefaultZoomLevel(Normal);
            return nullptr;
        }
        g_object_unref(info);
        QString zoomLevel = zoom_level;
        g_free(zoom_level);
        int level =(zoomLevel.toInt()) == Invalid? Normal: ZoomLevel(QString(zoomLevel).toInt());
        p_this->setDefaultZoomLevel(ZoomLevel(level));
        return nullptr;
    } else {
        if (err) {
            qDebug()<<err->code<<err->message;
            g_error_free(err);
        }
        p_this->setDefaultZoomLevel(Normal);
        return nullptr;
    }
}

bool AdvancedDesktopIconView::isFull()
{
    //bug#123045 主屏桌面空间不足，新建的文件未放置到扩展屏桌面上
    qDebug() << "colNum:" <<m_availableColumnCount << "rowNum:"<< m_availableRowCount << "total:" << m_proxy_model->rowCount();
    for (int column = 0; column < m_availableColumnCount; column++) {
        for (int row = 0; row < m_availableRowCount; row++) {
            QPoint pos = QPoint(column, row);
            if (m_itemPosHash.keys(pos).isEmpty())
                return false;
        }
    }
    return true;
}

void AdvancedDesktopIconView::setShowHidden()
{
    m_show_hidden = !GlobalSettings::getInstance()->getValue(SHOW_HIDDEN_PREFERENCE).toBool();
    m_proxy_model->setShowHidden(m_show_hidden);
    //fix show hidden file desktop icons overlapped issue
    //QTimer::singleShot(100, this, [=]() {
        //resetAllItemPositionInfos();
        //refresh();
    //});
    //fix#181595 桌面图标设置隐藏后排序
//    Q_EMIT updateView();
    m_proxy_model->invalidate();
    showHiddenFile();
}

void AdvancedDesktopIconView::showHiddenFile()
{
    if (m_proxy_model && m_proxy_model->getDesktopUseAutoLayout()) {
        relayoutExsitingItemsAndUpdate(m_autoArrange);
    }
    viewport()->update();
    return;
}

QPoint AdvancedDesktopIconView::checkGridPos(const QPoint pos)
{
    if (!isInvalidPoint(pos)) {
        return pos;
    }
    int col = pos.x() >= m_availableColumnCount ? m_availableColumnCount-1 : pos.x();
    int row = pos.y() >= m_availableRowCount ? m_availableRowCount-1 : pos.y();
    QPoint newPos = findNextEmptyGridPos(QPoint(col, row));
    return newPos;
}

void AdvancedDesktopIconView::onEntered(const QModelIndex &index)
{
    if (m_hoverIndex == index)
        return;

    if (m_hoverIndex.isValid()) {
        const QRect rect = visualRect(m_hoverIndex);
        if (viewport()->rect().intersects(rect))
            viewport()->update(rect);
    }

    if (index.isValid()) {
        const QRect rect = visualRect(index);
        if (viewport()->rect().intersects(rect))
            viewport()->update(rect);
    }

    m_hoverIndex = index;
}

bool AdvancedDesktopIconView::checkEmptyPositon()
{
    if (m_autoArrange.isEmpty())
        return false;
    for (int col = 0; col < m_availableColumnCount; col++) {
        for (int row = 0; row < m_availableRowCount; row++) {
            if (col * m_availableRowCount + row + 1 > m_autoArrange.count()) {
                return false;
            }
            if (m_itemPosHash.keys(QPoint(col, row)).isEmpty()) {
                return true;
            }
        }
    }

    return false;
}

bool AdvancedDesktopIconView::execSharedFileLink(const QString uri)
{
    auto info = FileInfo::fromUri(uri);
    if (info->isEmptyInfo()) {
        FileInfoJob j(info);
        j.querySync();
    }
    if (uri.endsWith(".desktop")) {
        GKeyFile* key_file = g_key_file_new();
        QUrl url = uri;
        QString desktopfp = url.path();
        g_key_file_load_from_file(key_file, desktopfp.toUtf8().constData(), G_KEY_FILE_KEEP_COMMENTS, nullptr);
        GError* error = NULL;
        if (g_key_file_has_key(key_file, G_KEY_FILE_DESKTOP_GROUP, "X-Peony-CMD", nullptr)) {
            if (g_key_file_has_key(key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_EXEC, nullptr)) {
                g_autofree char* val = g_key_file_get_value(key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_EXEC, &error);
                if (error) {
                    qWarning() << "get desktop file:" << uri << " name error:" << error->code << " -- " << error->message;
                    g_error_free(error);
                    error = nullptr;
                } else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
                    QProcess p;
                    p.setProgram("peony");
                    QString str = val;
                    str = str.replace("peony ","");
                    p.setArguments(QStringList() << str);
                    qint64 pid;
                    p.startDetached(&pid);
                    // send startinfo to kwindowsystem
                    quint32 timeStamp = QX11Info::isPlatformX11() ? QX11Info::appUserTime() : 0;
                    KStartupInfoId startInfoId;
                    startInfoId.initId(KStartupInfo::createNewStartupIdForTimestamp(timeStamp));
                    startInfoId.setupStartupEnv();
                    KStartupInfoData data;
                    data.setHostname();
                    data.addPid(pid);
#ifdef KSTARTUPINFO_HAS_SET_ICON_GEOMETRY
                    QRect rect = info.get()->property("iconGeometry").toRect();
                    if (rect.isValid()) {
                        data.setIconGeometry(rect);
                    }
#endif
                    data.setLaunchedBy(getpid());
                    KStartupInfo::sendStartup(startInfoId, data);
#else
                    QProcess p;
                    QString strq;
                    for (int i = 0;i < uri.length();++i) {
                        if(uri[i] == ' '){
                            strq += "%20";
                        }else{
                            strq += uri[i];
                        }
                    }
                    p.startDetached("/usr/bin/peony", QStringList()<<strq<<"%U&");
#endif
                    return true;
                }
            }
        }
    }
    return false;
}

int AdvancedDesktopIconView::radius() const
{
    return m_radius;
}

