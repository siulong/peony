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

#include "desktop-window-manager.h"
#include "desktopbackgroundwindow.h"
#include "advanced-desktop-icon-view.h"
#include "peony-desktop-application.h"
#include "advanced-desktop-item-model.h"
#include <KWindowSystem>

#include "common.h"
#include "file-info.h"
#include "file-info-job.h"
#include "file-meta-info.h"
#include "global-settings.h"
static int desktop_window_id = 0;
static DesktopWindowManager *global_instance = nullptr;

DesktopWindowManager *DesktopWindowManager::getInstance()
{
    if (!global_instance) {
        global_instance = new DesktopWindowManager;
    }
    return global_instance;
}

DesktopWindowManager::DesktopWindowManager(QObject *parent) : QObject(parent)
{
    m_model = PeonyDesktopApplication::getModel();
    connect(m_model, &AdvancedDesktopItemModel::emitFinish, this, &DesktopWindowManager::initModelFinish);
    m_desktopAutoLayout = GlobalSettings::getInstance()->getValue(DESKTOP_USE_AUTO_LAYOUT).toBool();
    connect(GlobalSettings::getInstance(), &GlobalSettings::valueChanged, this, [=](const QString &key){
        if (key == DESKTOP_USE_AUTO_LAYOUT) {
            m_desktopAutoLayout = GlobalSettings::getInstance()->getValue(DESKTOP_USE_AUTO_LAYOUT).toBool();
        }
    });
}


DesktopWindowManager::~DesktopWindowManager()
{
    for (auto window : m_bgWindows) {
        delete window;
    }
    m_bgWindows.clear();
}

void DesktopWindowManager::addBgWindow(QScreen *screen)
{
    int desktopWindowId = getDesktopWindowId();
    auto window = new DesktopBackgroundWindow(screen, desktopWindowId);

    m_bgWindows.append(window);
    desktop_window_id = m_bgWindows.count();

    qInfo()<<"[PeonyDesktopApplication::addBgWindow] screen name:"<<window->screen()->name()<<"  IP:"<<window->screen() << "count:" << m_bgWindows.count();
    window->show();

    if (m_initMetaInfo) {
        m_mode = checkScreenMode(screen->geometry());
        if (2 == m_mode) {
            multiscreenMode();
        } else {
            singleScreenMode();
        }
        relocateIconView();
    } else {
        //在设置模式后初始化viewprt，否则会导致主屏是扩展屏，插入扩展屏后，model没有数据
        if (screen == qApp->primaryScreen()) {
            window->getIconView()->refresh();
        }
    }

    //task#74174 恢复扩展屏
    connect(screen, &QScreen::destroyed, this, [=](){
        if (m_mode == 2) {
            if (m_bgWindows.count() > 2) {
                //task#74174 销毁时保存扩展屏元素的坐标点
                 m_model->saveExtendItemInfo(window->id());
                 Q_EMIT getIconView(qApp->primaryScreen())->getIconView()->updateView();
            } else if (m_bgWindows.count() == 2) {
                singleScreenMode();
                m_mode = 0;
            }
        }
        qInfo()<<"QScreen::destroyed screen name:"<<screen->name()<< m_bgWindows.count();
        window->invaidScreen();
        bool sucess = m_bgWindows.removeOne(window);
        qDebug()<<"QScreen::destroyed :"<<sucess;
        delete window;
    });
//    connect(screen, &QScreen::destroyed, this, [=](){
//        removeWindow(window);
//    });

    connect(window, &DesktopBackgroundWindow::setDefaultZoomLevel, this, [=](AdvancedDesktopIconView::ZoomLevel level){
        for (auto bgWindow : m_bgWindows) {
            if (bgWindow->getIconView()->zoomLevel() != level) {
                bgWindow->getIconView()->setDefaultZoomLevel(level);
            }
        }
    });
    //task#74174 更新排序方式
    connect(window, &DesktopBackgroundWindow::setSortType, this, [=](int sortType){
        clearAllRestoreInfo();
        for (auto bgWindow : m_bgWindows) {
            bgWindow->getIconView()->setSortType(sortType);
        }
    });
    //task#74174 更新扩展屏与镜像切换
    connect(window, &DesktopBackgroundWindow::updateWindow, this, [=](const QRect &geometry){
        int mode = checkScreenMode(geometry);
        if (m_mode != mode) {
            if (1 == mode) {
                singleScreenMode();
             } else if (2 == mode) {
                multiscreenMode();
            }
        }
        m_mode = mode;
        window->setWindowGeometry(geometry);
    });
    connect(window, &DesktopBackgroundWindow::markFilePos, this, &DesktopWindowManager::markCreateFilePos);

    connect(window, &DesktopBackgroundWindow::clearOtherViewSelection, [=](){
        for (auto bgWindow : m_bgWindows) {
            if (window == bgWindow) {
                continue;
            }
            auto view = bgWindow->getIconView();
            view->clearAllIndexWidgets();
            view->clearSelection();
        }
    });

    connect(window, &DesktopBackgroundWindow::setSortOrder, this, [=](int sortOrder){
        for (auto bgWindow : m_bgWindows) {
            bgWindow->getIconView()->setSortOrder(sortOrder);
        }
    });

}

void DesktopWindowManager::removeWindow(DesktopBackgroundWindow *window)
{
    QScreen *screen = qobject_cast<QScreen *>(sender());
    qInfo()<<"QScreen::destroyed screen name:"<<screen->name()<< m_bgWindows.count();
    if (m_mode == 2) {
        if (m_bgWindows.count() > 2) {
            //task#74174 销毁时保存扩展屏元素的坐标点
             m_model->saveExtendItemInfo();
        } else if (m_bgWindows.count() == 2) {
            singleScreenMode();
            m_mode = 0;
        }
    }
    bool sucess = m_bgWindows.removeOne(window);
    qDebug()<<"QScreen::destroyed :"<<sucess;
    delete window;
}

void DesktopWindowManager::singleScreenMode()
{
    auto primayView = getIconView(0);

    qDebug() << "primay view item" << primayView->model()->rowCount() << "total item:" << m_model->rowCount();
    if ( 2 == m_mode  && primayView->model()->rowCount() == m_model->rowCount()) {
        m_model->clearExtendItemPos();
        return;
    }
    m_model->getAllRestoreInfo();
    m_model->saveExtendItemInfo();
    for (auto bgWindow : m_bgWindows) {
        auto view = bgWindow->getIconView();
        Q_EMIT view->updateView();
    }
}

void DesktopWindowManager::multiscreenMode()
{
    m_model->getAllRestoreInfo();
    m_model->resetExtendItemInfo();

    for (auto bgWindow : m_bgWindows) {
        auto view = bgWindow->getIconView();
        Q_EMIT view->updateView();
    }
}

void DesktopWindowManager::relocateIconView()
{
    qInfo()<<"start relocate icon view";
    //task#74174 更新多屏显示,根据id过滤元素
    int id = -1;
    DesktopBackgroundWindow *primaryWindow = nullptr;
    for (auto window : m_bgWindows) {
        if (window->screen() == qApp->primaryScreen()) {
            id = window->id();
            primaryWindow = window;
            break;
        }
    }
    qDebug()<<"primary screen id:"<<id;
    if (0 < id) {
        for (auto window : m_bgWindows) {
            if (0 == window->id()) {
                m_model->getAllRestoreInfo();
                window->setId(id);
                primaryWindow->setId(0);
                Q_EMIT window->getIconView()->updateView();
                Q_EMIT primaryWindow->getIconView()->updateView();
                KWindowSystem::raiseWindow(primaryWindow->winId());
                return;
            }
        }
    }

    for (auto window : m_bgWindows) {
        qDebug() << "screen name :" << window->screen()->name() << " id:" << window->id() ;
        if (window->screen() != qApp->primaryScreen()) {
            KWindowSystem::raiseWindow(window->winId());
        }
    }
    if(primaryWindow) {
        KWindowSystem::raiseWindow(primaryWindow->winId());
    }
}

int DesktopWindowManager::checkScreenMode(const QRect &geometry)
{
    int mode = 1;
    if (m_bgWindows.count() == 1) {
        mode = 0;
        return mode;
    }

    for (auto window : m_bgWindows) {
        if (window->screen()->geometry() != geometry) {
            mode = 2;
            break;
        }
    }
    return mode;
}

int DesktopWindowManager::getDesktopWindowId()
{
    int desktopWindowId = 0;
    for (; desktopWindowId <= desktop_window_id; desktopWindowId++) {
        bool find = false;
        for (auto bgWindow : m_bgWindows) {
            if (bgWindow->id() == desktopWindowId) {
                find = true;
                break;
            }
        }
        if (!find) {
            break;
        }
    }
    return desktopWindowId;
}

void DesktopWindowManager::raiseWid()
{
    QTimer::singleShot(2000, this, [=]() {
        for (auto window : m_bgWindows) {
            if (window->screen() == QGuiApplication::primaryScreen() && window->screen()) {
                KWindowSystem::raiseWindow(window->winId());
                return;
            }
        }
    });
}

DesktopBackgroundWindow *DesktopWindowManager::getIconView(QPoint pos)
{
    //获取当前屏幕的view,如果是镜像直接返回主屏
    DesktopBackgroundWindow *window = getIconView(qApp->primaryScreen());
    QRegion screenRegion(qApp->primaryScreen()->geometry());
    if (screenRegion.contains(pos)) {
        return window;
    };

    for (auto bgWindow : m_bgWindows) {
        QRegion screenRegion(bgWindow->screen()->geometry());
        if (screenRegion.contains(pos)) {
            window = bgWindow;
            break;
        };
    }
    return window;
}

AdvancedDesktopIconView *DesktopWindowManager::getIconView(int id)
{
    for (auto window : m_bgWindows) {
        if (id == window->id()) {
            return window->getIconView();
        }
    }
    return m_bgWindows[0]->getIconView();
}

DesktopBackgroundWindow *DesktopWindowManager::getIconView(QScreen *screen)
{
    for (auto window : m_bgWindows) {
        if (screen == window->screen()) {
            return window;
        }
    }
    return m_bgWindows[0];
}

void DesktopWindowManager::layoutViewItems()
{
    for (auto bgWindow : m_bgWindows) {
        auto view = bgWindow->getIconView();
        view->layoutItems();
    }
}

void DesktopWindowManager::initModelFinish()
{
    if (m_initMetaInfo) {
        return;
    }

    if (m_bgWindows.count() < 1)
        return;

    m_mode = checkScreenMode(m_bgWindows[0]->screen()->geometry());
    if (2 == m_mode) {
        multiscreenMode();
    } else {
        singleScreenMode();
    }
    relocateIconView();
    m_initMetaInfo = true;

    Q_EMIT emitFinish();
}

//QPoint DesktopWindowManager::postion(const QString &uri)
//{
//    for (int i = 0; i < m_model->rowCount(); i++) {
//        auto index = m_model->index(i, 0);
//        QString currentUri = index.data(Qt::UserRole).toString();
//        if (uri == currentUri) {
////            auto metaInfo = FileMetaInfo::fromUri(uri);
////            if (metaInfo) {
////                QStringList strPos = metaInfo->getMetaInfoStringList(ITEM_GRID_POS_ATTRIBUTE);
////                if (strPos.count() == 2) {
////                    QPoint pos(strPos[0].toInt(), strPos[1].toInt());
////                    return pos;
////                }
////            }
//            QPoint pos = index.data(AdvancedDesktopItemModel::PositionRole).toPoint();
//            return pos;
//        }
//    }
//    return QPoint(-1, -1);
//}

//QMap<QString, QPoint> DesktopWindowManager::getAllItemPos(const QStringList &uris)
//{
//    QMap<QString, QPoint> itemPosMap;
//    Peony::AdvancedDesktopItemModel *model = PeonyDesktopApplication::getModel();
//    for (int i = 0; i < model->rowCount(); i++) {
//        auto index = model->index(i, 0);
//        QString currentUri = index.data(Qt::UserRole).toString();
//        if (uris.contains(currentUri)) {
////            auto metaInfo = FileMetaInfo::fromUri(currentUri);
////            if (metaInfo) {
////                QStringList strPos = metaInfo->getMetaInfoStringList(ITEM_GRID_POS_ATTRIBUTE);
////                if (strPos.count() == 2) {
////                    QPoint pos(strPos[0].toInt(), strPos[1].toInt());

////                }
////            }
//            QPoint pos = index.data(AdvancedDesktopItemModel::PositionRole).toPoint();
//            itemPosMap.insert(currentUri, pos);
//        }
//    }
//    return itemPosMap;
//}

void DesktopWindowManager::markCreateFilePos(const QPoint &pos)
{
    //如果当前创建文件的屏幕满了，则找到一个空的屏幕，按照顺序添加
    DesktopBackgroundWindow *window = qobject_cast<DesktopBackgroundWindow *>(sender());
    //鼠标坐标点转化成网格标点；
    QPoint tmpPos = window->getIconView()->getGridPosFromMousePos(pos);
    m_markMousePressPos = window->getIconView()->checkGridPos(tmpPos);
    m_markMousePressId = window->id();
    if (window->getIconView()->isFull()) {
        for (auto bgWindow : m_bgWindows) {
            bool isFull = bgWindow->getIconView()->isFull();
            if (!isFull) {
                m_markMousePressPos = QPoint(0, 0);
                m_markMousePressId = bgWindow->id();
//                bgWindow->getIconView()->markFilePos(m_markMousePressPos);
                return;
            }
        }
        m_markMousePressPos = QPoint(0, 0);
        m_markMousePressId = window->id();
    }
//    window->getIconView()->markFilePos(m_markMousePressPos);
}

void DesktopWindowManager::createIdAndPos(int *id, QPoint *pos)
{
    if (m_markMousePressPos == QPoint(-1, -1)) {
        auto window = getIconView(QCursor::pos());
        bool isFull = window->getIconView()->isFull();
        if (!isFull) {
            *id = window->id();
            *pos = m_markMousePressPos;
            return;
        }
        for (auto window : m_bgWindows) {
            bool isFull = window->getIconView()->isFull();
            if (!isFull) {
                *id = window->id();
                *pos = m_markMousePressPos;
                return;
            }
        }
        *id = window->id();
        *pos = QPoint(0, 0);
        return;
    }

    *id = m_markMousePressId;
    *pos = m_desktopAutoLayout? QPoint(0, 0): m_markMousePressPos;//自动排列的情况下清空位置的记录
    m_markMousePressPos = QPoint(-1, -1);
    m_markMousePressId = 0;
    return;
}

void DesktopWindowManager::clearAllRestoreInfo()
{
    for (int i = 0; i < m_model->rowCount(); i++) {
        auto index = m_model->index(i, 0);
        m_model->setData(index, QStringList(""), AdvancedDesktopIconView::ExceptedPositionRole);
    }
}

