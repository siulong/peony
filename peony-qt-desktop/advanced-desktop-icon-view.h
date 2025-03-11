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

#ifndef ADVANCEDDESKTOPICONVIEW_H
#define ADVANCEDDESKTOPICONVIEW_H

#include <QAbstractItemView>
#include <QHash>
#include <gio/gio.h>

#include "directory-view-plugin-iface.h"
#include "peony-dbus-service.h"

#include <QStandardPaths>
#include <QTimer>

#include <QMap>

class QRubberBand;
class QGSettings;

namespace  Peony {
class AdvancedDesktopItemModel;
class DesktopItemProxyModel;
class DesktopWindow;
class DesktopIndexWidget;
class DesktopIconViewDelegate;
class FileItemModel;
class PeonyDbusService;
}

class AdvancedDesktopIconView : public QAbstractItemView, public Peony::DirectoryViewIface
{
    friend class Peony::DesktopWindow;
    friend class Peony::DesktopIndexWidget;
    friend class Peony::DesktopIconViewDelegate;
    friend class Peony::AdvancedDesktopItemModel;
    Q_OBJECT
public:
    enum ZoomLevel {
        Invalid,
        Small, //icon: 24x24; grid: 64x64; hover rect: 60x60; font: system*0.8
        Normal, //icon: 48x48; grid: 96x96; hover rect = 90x90; font: system
        Large, //icon: 64x64; grid: 115x135; hover rect = 105x118; font: system*1.2
        Huge //icon: 96x96; grid: 140x170; hover rect = 120x140; font: system*1.4

    };
    Q_ENUM(ZoomLevel)

    enum Direction {
        All,
        Right,
        Bottom
    };
    Q_ENUM(Direction)

    enum DesktopIconRole {
        UriRole = Qt::UserRole,
        PositionRole = Qt::UserRole + 2,
        ScreenIdRole = Qt::UserRole + 3,
        ExceptedPositionRole = Qt::UserRole + 4,
        SinglesSreenPositionRole = Qt::UserRole + 5,
        ExtendScreenPositionRole = Qt::UserRole + 6,
        IsFloatItemRole = Qt::UserRole + 7
    }; Q_ENUM (DesktopIconRole)

    explicit AdvancedDesktopIconView(QWidget *parent = nullptr);
    ~AdvancedDesktopIconView();
    void initShoutCut();

    void openFileByUri(QString uri);
    void initDoubleClick();

    void bindModel(Peony::FileItemModel *sourceModel, Peony::FileItemProxyFilterSortModel *proxyModel) {
        Q_UNUSED(sourceModel) Q_UNUSED(proxyModel)
    }
    void setProxy(Peony::DirectoryViewProxyIface *proxy) {
        Q_UNUSED(proxy)
    }

    const QString viewId() {
        return tr("Desktop Icon View");
    }

    //location
    const QString getDirectoryUri() {
        return "file://" + QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }


    QRect visualRect(const QModelIndex &index) const override;
    void scrollTo(const QModelIndex &index, ScrollHint hint) override;
    QModelIndex indexAt(const QPoint &point) const override;

    //selections
    const QStringList getSelections();

    //children
    const QStringList getAllFileUris();
    const int getAllDisplayFileCount();
    void UpdateToEditUris(QStringList uris);
    ZoomLevel zoomLevel() const;
    void setEditFlag(bool edit);
    bool getEditFlag();

    bool isRenaming();
    void setRenaming(bool renaming);
    int updateBWList();
    QString getBlackAndWhiteModel();
    QSet<QString> getBWListInfo();
    bool getBlackAndWhiteListExist(QString name);
    void clearAllIndexWidgets(const QStringList &uris = QStringList());

    void setId(int id);
    bool isFull();

    int radius() const;

private:
    bool execSharedFileLink(const QString uri);

Q_SIGNALS:
    void updateView();
    void sig_fileCreated(const QString &uri);
    void clearOtherViewSelection();
    void setZoomLevel(ZoomLevel level);

public Q_SLOTS:

    //location
    void open(const QStringList &uris, bool newWindow) {}
    void setDirectoryUri(const QString &uri) {}
    void beginLocationChange() {}
    void stopLocationChange() {}
    void closeView();

    //selections
    void setSelections(const QStringList &uris);
    void invertSelections();
    void scrollToSelection(const QString &uri);

    //clipboard
    void setCutFiles(const QStringList &uris);

    Peony::DirectoryViewProxyIface *getProxy() {
        return nullptr;
    }

    void setSortType(int sortType);

    void setSortOrder(int sortOrder);

    //edit
    void editUri(const QString &uri);

    int getSortType();
    int getSortOrder();

    void editUris(const QStringList uris);
    void fileCreated(const QString &uri);

    void setDefaultZoomLevel(ZoomLevel level);
    void zoomIn();
    void zoomOut();

    /**
     * @brief layoutItems
     * 自动排列
     * @param closest
     * @return
     * 返回自动排列的uri列表
     */
    QStringList layoutItems(bool closest = false);

    /**
     * @brief relayoutExsitingItemsAndUpdate
     * 设置自动排列后，
     * @param uris
     */
    void relayoutExsitingItemsAndUpdate(const QStringList uris);

    void setShowHidden();
    void showHiddenFile();

protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    int horizontalOffset() const override;
    int verticalOffset() const override;
    bool isIndexHidden(const QModelIndex &index) const override;
    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override;
    QRegion visualRegionForSelection(const QItemSelection &selection) const override;

    QStyleOptionViewItem viewOptions() const override;
    void startDrag(Qt::DropActions supportedActions) override;

    void reset() override;
    void rowsInserted(const QModelIndex &parent, int start, int end) override;
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) override;

    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event);

    //void ensureItemPosByUri(const QString &uri);
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    bool viewportEvent(QEvent *e) override;

    QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex &index, const QEvent *event) const override;

    bool dropWhenAotoArrange(QDropEvent *event);
    bool dragToOtherScreen(QDropEvent *event);

    void wheelEvent(QWheelEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);

    void checkItemsOver();
    static GAsyncReadyCallback queryZoomLevelAsyncCallback(GObject *obj,
            GAsyncResult *res,
            AdvancedDesktopIconView *p_this);

private Q_SLOTS:
    void recalculateAvailableRowAndColumnCount();
    void onEntered(const QModelIndex &index);

public:
    int getViewAvailableRowCount();
    int getViewAvailableColumnCount();

    void setMargins();
    /*!
     * \brief getIndexGridPos
     * 获取当前index所在的网格位置
     * \param index
     * \param ok
     * 如果成功获取则设置为true，否则设置为false
     * \return
     * index所在的网格位置，如果不成功则返回（-1，-1）
     */
    QPoint getIndexGridPos(const QModelIndex &index, bool *ok) const;

    /*!
     * \brief findNextEmptyGridPos
     * 获取下一个空网格所在的位置
     * \return
     *
     */
    QPoint findNextEmptyGridPos(const QPoint &startGridPos = QPoint());

    bool isInvalidPoint(const QPoint &point);
    QPoint invalidPoint() const;

    /*!
     * \brief getGridPosFromMousePos
     * 根据当前鼠标事件传入的位置返回网格的位置
     * \param mousePos
     * \return
     * 鼠标位置对应的网格位置
     */
    QPoint getGridPosFromMousePos(const QPoint &mousePos);

    /*!
     * \brief gridPosLesserThan
     * 从左上到右下比较两个网格位置的顺序
     * \param leftPos
     * \param rightPos
     * \return
     */
    bool gridPosLesserThan(const QPoint &leftPos, const QPoint &rightPos);
    void refresh();

    void setFileMetaInfoPos(const QString &uri, const QPoint &pos);
    void relayoutExsitingItems(const QStringList &uris);
    void resolutionChange();
    QPoint checkGridPos(const QPoint pos);
    bool checkEmptyPositon();

private:
    ZoomLevel m_zoom_level = Invalid;
    QSize m_gridSize = QSize();
    int m_availableRowCount = 1;
    int m_availableColumnCount = 1;
    QRubberBand *m_rubberBand = nullptr;
    QPoint m_pressedPos = QPoint(-1,-1);
    QPoint m_dragPressPos;
    /*!
     * \brief m_itemPosMap
     * 当前视图中的文件uri和对应所在的网格位置
     */
    QHash<QString, QPoint> m_itemPosHash;
    QStringList m_autoArrange;
    QHash<QString, QPoint> m_resolutionItemPosHash;
    QStringList m_storageBox;
//    QVector<bool> m_itemStatus;
    QModelIndex m_last_index;
    QTimer m_edit_trigger_timer;

    QStringList m_new_files_to_be_selected;
    QStringList m_uris_to_edit;/* 新建文件/文件夹，可编辑文件名list */
    QString m_edit_uri;    /* 正在编辑的文件，信息更新不重置*/

   // bool m_is_refreshing = false;

    bool m_real_do_edit = false;

    bool m_ctrl_or_shift_pressed = false;

    bool  m_ctrl_key_pressed = false;

    bool m_shift_key_pressed = false;

    bool m_show_hidden;

    bool m_is_renaming = false;

    bool m_is_edit = false;

    bool m_initialized = false;
    bool m_isDragging = false;

    bool m_noSelectOnPress = false;

    Peony::AdvancedDesktopItemModel *m_model = nullptr;
    Peony::DesktopItemProxyModel *m_proxy_model = nullptr;
    QGSettings *m_panelSetting = nullptr;
    Peony::PeonyDbusService *m_peonyDbusSer = nullptr;
    int m_id = 0;
    QRect m_dropIndicatorRect;
    DropIndicatorPosition m_dropIndicatorPos = OnViewport;
    QPersistentModelIndex m_hoverIndex;
    QPoint m_margin;

    int m_radius = 6;
};

#endif // ADVANCEDDESKTOPICONVIEW_H
