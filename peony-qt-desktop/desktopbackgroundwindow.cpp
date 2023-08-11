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

#include "desktopbackgroundwindow.h"
#include "desktop-background-manager.h"
#include "peony-desktop-application.h"
#include "desktop-menu.h"
#include <QScreen>
#include <QPainter>
#include <QVariantAnimation>
#include <QTimeLine>
#include <KWindowSystem>
#include <QPlatformSurfaceEvent>
#include "plasma-shell-manager.h"

#include <QRegion>

#include <QX11Info>
#include <X11/Xlib.h>

static QTimeLine *gTimeLine = nullptr;

static bool startup = false;

DesktopBackgroundWindow::DesktopBackgroundWindow(QScreen *screen, int desktopWindowId, QWidget *parent) : QMainWindow(parent)
{
    if (!gTimeLine) {
        gTimeLine = new QTimeLine(100);
    }
    connect(gTimeLine, &QTimeLine::finished, this, &DesktopBackgroundWindow::updateWindowGeometry);
    setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::Window|Qt::FramelessWindowHint);
    KWindowSystem::setType(this->winId(), NET::Desktop);
    KWindowSystem::setState(this->winId(), NET::SkipTaskbar|NET::SkipPager|NET::SkipSwitcher);

    setContextMenuPolicy(Qt::CustomContextMenu);

    m_screen = screen;
    m_desktopIconView = new Peony::DesktopIconView(this);
    m_desktopIconView->setId(desktopWindowId);
    m_id = desktopWindowId;
    move(screen->geometry().topLeft());
    setFixedSize(screen->geometry().size());
    setContentsMargins(0, 0, 0, 0);
    m_desktopIconView->resize(screen->geometry().size());
    connect(screen, &QScreen::geometryChanged, this, QOverload<const QRect&>::of(&DesktopBackgroundWindow::updateWindow));

    auto manager = DesktopBackgroundManager::globalInstance();
    connect(manager, &DesktopBackgroundManager::screensUpdated, this, QOverload<>::of(&DesktopBackgroundWindow::update));

    if (QGSettings::isSchemaInstalled("org.ukui.panel.settings")) {
        m_panelSetting = new QGSettings("org.ukui.panel.settings", QByteArray(), this);
    }

    connect(this, &QWidget::customContextMenuRequested, this, [=](const QPoint &pos){
        QPoint relativePos = getRelativePos(pos);
        qInfo()<<pos;
        // fix #115384, context menu key issue
//        auto index = PeonyDesktopApplication::getIconView()->indexAt(relativePos);
//        if (!index.isValid()) {
//            PeonyDesktopApplication::getIconView()->clearSelection();
//        } else {
//            if (!PeonyDesktopApplication::getIconView()->selectionModel()->selection().indexes().contains(index)) {
//                PeonyDesktopApplication::getIconView()->clearSelection();
//                PeonyDesktopApplication::getIconView()->selectionModel()->select(index, QItemSelectionModel::Select);
//            }
//        }

        QTimer::singleShot(1, [=]() {
           //task#74174  扩展屏设置菜单
            if (m_menu) {
                delete m_menu;
                m_menu = nullptr;
            }
            m_menu = new DesktopMenu(m_desktopIconView, this);
            connect(m_menu, &DesktopMenu::setDefaultZoomLevel, this, &DesktopBackgroundWindow::setDefaultZoomLevel);
            connect(m_menu, &DesktopMenu::setSortType, this, &DesktopBackgroundWindow::setSortType);

            if (m_desktopIconView->getSelections().isEmpty()) {
                auto action = m_menu->addAction(QObject::tr("set background"));
                connect(action, &QAction::triggered, [=]() {
                    //go to control center set background
                    PeonyDesktopApplication::gotoSetBackground();
                });
                auto action1 = m_menu->addAction(QObject::tr("display settings"));
                connect(action1, &QAction::triggered, [=]() {
                    //go to control center set resolution ratio
                    PeonyDesktopApplication::gotoSetResolution();
                });

            }

            for (auto screen : qApp->screens()) {
                if (screen->geometry().contains(relativePos));
                //menu.windowHandle()->setScreen(screen);
            }
            m_menu->exec(mapToGlobal(pos));
            auto urisToEdit = m_menu->urisToEdit();
            m_desktopIconView->UpdateToEditUris(urisToEdit);
//            if (urisToEdit.count() >= 1) {
//                QTimer::singleShot(
//                            100, this, [=]() {
//                    m_desktopIconView->editUri(urisToEdit.first());
//                    qDebug() << "editUri count >=1:"<<urisToEdit.first();
//                });
//            }
        });
    });
}

DesktopBackgroundWindow::~DesktopBackgroundWindow()
{

}

void DesktopBackgroundWindow::paintEvent(QPaintEvent *event)
{
    auto manager = DesktopBackgroundManager::globalInstance();
    if (!manager->getPaintBackground())
        return;

    if (!m_screen)
        return;

    QPainter p(this);
    if (manager->getUsePureColor()) {
        p.fillRect(this->rect(), manager->getColor());
    } else {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        auto animation = manager->getAnimation();
        QPixmap frontPixmap = manager->getFrontPixmap();

        if (animation->state() == QVariantAnimation::Running) {
            auto opacity = animation->currentValue().toReal();
            QPixmap backPixmap = manager->getBackPixmap();

            if (manager->getBackgroundOption() == "centered") {
                //居中
                p.drawPixmap((m_screen->size().width() - backPixmap.rect().width()) / 2,
                             (m_screen->size().height() - backPixmap.rect().height()) / 2,
                             backPixmap);
                p.setOpacity(opacity);
                p.drawPixmap((m_screen->size().width() - frontPixmap.rect().width()) / 2,
                             (m_screen->size().height() - frontPixmap.rect().height()) / 2,
                             frontPixmap);
            } else if (manager->getBackgroundOption() == "stretched") {
                //拉伸
                p.drawPixmap(this->rect(), backPixmap, backPixmap.rect());
                p.setOpacity(opacity);
                p.drawPixmap(this->rect(), frontPixmap, frontPixmap.rect());
            } else if (manager->getBackgroundOption() == "scaled") {
                //填充
                p.drawPixmap(this->rect(), backPixmap, getSourceRect(backPixmap));
                p.setOpacity(opacity);
                p.drawPixmap(this->rect(), frontPixmap, getSourceRect(frontPixmap));
            } else if (manager->getBackgroundOption() == "wallpaper") {
                //平铺
                int drawedWidth = 0;
                int drawedHeight = 0;
                while (1) {
                    drawedWidth = 0;
                    while (1) {
                        p.drawPixmap(drawedWidth, drawedHeight, backPixmap);
                        drawedWidth += backPixmap.width();
                        if (drawedWidth >= m_screen->size().width()) {
                            break;
                        }
                    }
                    drawedHeight += backPixmap.height();
                    if (drawedHeight >= m_screen->size().height()) {
                        break;
                    }
                }
                p.setOpacity(opacity);
                drawedWidth = 0;
                drawedHeight = 0;
                while (1) {
                    drawedWidth = 0;
                    while (1) {
                        p.drawPixmap(drawedWidth, drawedHeight, frontPixmap);
                        drawedWidth += frontPixmap.width();
                        if (drawedWidth >= m_screen->size().width()) {
                            break;
                        }
                    }
                    drawedHeight += frontPixmap.height();
                    if (drawedHeight >= m_screen->size().height()) {
                        break;
                    }
                }
            } else if (manager->getBackgroundOption() == "zoom") {
                //适应
                p.drawPixmap(getDestRect(backPixmap), backPixmap, backPixmap.rect());
                p.setOpacity(opacity);
                p.drawPixmap(getDestRect(frontPixmap), frontPixmap, frontPixmap.rect());
            } else if (manager->getBackgroundOption() == "spanned") {
                //跨区
                p.drawPixmap(this->rect(), backPixmap, getSourceRect(backPixmap, m_screen->geometry()));
                p.setOpacity(opacity);
                p.drawPixmap(this->rect(), frontPixmap, getSourceRect(frontPixmap, m_screen->geometry()));
            } else {
                p.drawPixmap(rect().adjusted(0, 0, -1, -1), backPixmap, backPixmap.rect());
                p.setOpacity(opacity);
                p.drawPixmap(rect().adjusted(0, 0, -1, -1), frontPixmap, frontPixmap.rect());
            }

        } else {
            if (manager->getBackgroundOption() == "centered") {
                p.drawPixmap((m_screen->size().width() - frontPixmap.rect().width()) / 2,
                             (m_screen->size().height() - frontPixmap.rect().height()) / 2,
                             frontPixmap);
            } else if (manager->getBackgroundOption() == "stretched") {
                p.drawPixmap(this->rect(), frontPixmap, frontPixmap.rect());

            } else if (manager->getBackgroundOption() == "scaled") {
                p.drawPixmap(this->rect(), frontPixmap, getSourceRect(frontPixmap));

            } else if (manager->getBackgroundOption() == "wallpaper") {
                int drawedWidth = 0;
                int drawedHeight = 0;
                while (1) {
                    drawedWidth = 0;
                    while (1) {
                        p.drawPixmap(drawedWidth, drawedHeight, frontPixmap);
                        drawedWidth += frontPixmap.width();
                        if (drawedWidth >= m_screen->size().width()) {
                            break;
                        }
                    }
                    drawedHeight += frontPixmap.height();
                    if (drawedHeight >= m_screen->size().height()) {
                        break;
                    }
                }
            } else if (manager->getBackgroundOption() == "zoom") {
                p.drawPixmap(getDestRect(frontPixmap), frontPixmap, frontPixmap.rect());
            } else if (manager->getBackgroundOption() == "spanned") {
                p.drawPixmap(this->rect(), frontPixmap, getSourceRect(frontPixmap, m_screen->geometry()));
            } else {
                p.drawPixmap(rect().adjusted(0, 0, -1, -1), frontPixmap, frontPixmap.rect());
            }
        }
        p.restore();
    }

    if (!startup) {
        startup = true;
        QTimer::singleShot(1000, []{
            if (QX11Info::isPlatformX11()) {
                XSetWindowBackground(QX11Info::display(), QX11Info::appRootWindow(), 0);
                XSync(QX11Info::display(), false);
            }
        });
    }
}

QScreen *DesktopBackgroundWindow::screen() const
{
    return m_screen;
}

void DesktopBackgroundWindow::invaidScreen()
{
    m_screen = nullptr;
}

bool DesktopBackgroundWindow::event(QEvent *event)
{
    if (event->type() == QEvent::PlatformSurface) {
        auto e = static_cast<QPlatformSurfaceEvent *>(event);
        switch (e->surfaceEventType()) {
        case QPlatformSurfaceEvent::SurfaceCreated: {
            m_shellSurface = PlasmaShellManager::getInstance()->createSurface(this->windowHandle());
            if (m_shellSurface) {
                m_shellSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Desktop);
                m_shellSurface->setSkipSwitcher(true);
                m_shellSurface->setSkipTaskbar(true);
                // wayland中构造函数的move只能在这里生效
                if (m_screen) {
                    m_shellSurface->setPosition(m_screen->geometry().topLeft());
                }
            }
            break;
        }
        case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed: {
            if (m_shellSurface) {
                m_shellSurface->deleteLater();
                m_shellSurface = nullptr;
            }
            break;
        }
        default:
            break;
        }
    }
    return QMainWindow::event(event);
}

void DesktopBackgroundWindow::setWindowGeometry(const QRect &geometry)
{
    qInfo()<<"bg window geometry changed"<<screen()->name()<<geometry<<screen()->geometry();
    if (gTimeLine->state() != QTimeLine::Running) {
        gTimeLine->start();
    } else {
        gTimeLine->setCurrentTime(0);
    }
}

void DesktopBackgroundWindow::updateWindowGeometry()
{
    if (!m_screen) {
        return;
    }
    auto geometry = m_screen->geometry();
    move(geometry.topLeft());
    if (m_shellSurface) {
        m_shellSurface->setPosition(geometry.topLeft());
    }
    setFixedSize(geometry.size());

    qInfo()<<"bg window geometry changed slot"<<screen()->name()<<geometry;

    // raise primary window to make sure icon view is visible.
    if (centralWidget()) {
        if (screen() == qApp->primaryScreen()) {
            qInfo()<<"has center widget, raise window";
            KWindowSystem::raiseWindow(this->winId());
        } else {
            qCritical()<<"raise a window which not in primary screen, but has central widget";
        }
    }
    update();
}

int DesktopBackgroundWindow::id() const
{
    return m_id;
}

void DesktopBackgroundWindow::setId(int id)
{
    m_id = id;
    m_desktopIconView->setId(id);
}

//获取iconview中图标的相对位置
QPoint DesktopBackgroundWindow::getRelativePos(const QPoint &pos)
{
    if (!m_screen) {
        return pos;
    }
    QPoint relativePos = pos;
    if (m_screen == QApplication::primaryScreen()) {
        if (m_panelSetting) {
            int position = m_panelSetting->get("panelposition").toInt();
            int offset = m_panelSetting->get("panelsize").toInt();

            switch (position) {
                case 1: {
                    relativePos -= QPoint(0, offset);
                    break;
                }
                case 2: {
                    relativePos -= QPoint(offset, 0);
                    break;
                }
                case 3: {
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }

    return relativePos;
}

QRect DesktopBackgroundWindow::getSourceRect(const QPixmap &pixmap)
{
    qreal screenScale = qreal(this->rect().width()) / qreal(this->rect().height());
    qreal width = pixmap.width();
    qreal height = pixmap.height();

    if ((width / height) == screenScale) {
        return pixmap.rect();
    }

    bool isShortX = (width <= height);
    if (isShortX) {
        screenScale = qreal(this->rect().height()) / qreal(this->rect().width());
    }

    qreal shortEdge = isShortX ? width : height;
    qreal longEdge = isShortX ? height : width;

    while (shortEdge > 1) {
        qint32 temp = qFloor(shortEdge * screenScale);
        if (temp <= longEdge) {
            longEdge = temp;
            break;
        }

        qint32 spacing = qRound(shortEdge / 20);
        if (spacing <= 0) {
            spacing = 1;
        }
        shortEdge -= spacing;
    }

    QSize sourceSize = pixmap.size();
    if (shortEdge > 1 && longEdge > 1) {
        sourceSize.setWidth(isShortX ? shortEdge : longEdge);
        sourceSize.setHeight(isShortX ? longEdge : shortEdge);
    }

    qint32 offsetX = 0;
    qint32 offsetY = 0;
    if (pixmap.width() > sourceSize.width()) {
        offsetX = (pixmap.width() - sourceSize.width()) / 2;
    }

    if (pixmap.height() > sourceSize.height()) {
        offsetY = (pixmap.height() - sourceSize.height()) / 2;
    }

    QPoint offsetPoint = pixmap.rect().topLeft();
    offsetPoint += QPoint(offsetX, offsetY);

    return QRect(offsetPoint, sourceSize);
}

QRect DesktopBackgroundWindow::getSourceRect(const QPixmap &pixmap, const QRect &screenGeometry)
{
    QRegion region;
    for (auto screen : qApp->screens()) {
        region += screen->geometry();
    }
    QRect virtualGeometry = region.boundingRect().translated(0, 0);
    qreal pixWidth = pixmap.width();
    qreal pixHeight = pixmap.height();

    QSize sourceSize = pixmap.size();
    sourceSize.setWidth(screenGeometry.width() * 1.0 / virtualGeometry.width() * pixWidth);
    sourceSize.setHeight(screenGeometry.height() * 1.0 / virtualGeometry.height() * pixHeight);

    qint32 offsetX = 0;
    qint32 offsetY = 0;
    if (screenGeometry.x() > 0) {
        offsetX = (screenGeometry.x() * 1.0 / virtualGeometry.width() * pixWidth);
    }

    if (screenGeometry.y() > 0) {
        offsetY = (screenGeometry.y() * 1.0 / virtualGeometry.height() * pixHeight);
    }

    QPoint offsetPoint = pixmap.rect().topLeft();
    offsetPoint += QPoint(offsetX, offsetY);

    return QRect(offsetPoint, sourceSize);
}

QRect DesktopBackgroundWindow::getDestRect(const QPixmap &pixmap)
{
    qreal screenScale = qreal(this->rect().width()) / qreal(this->rect().height());
    qreal pixmapScale = qreal(pixmap.width() / pixmap.height());
    qreal width = pixmap.width();
    qreal height = pixmap.height();

    if (pixmapScale == screenScale) {
        return this->rect();
    }

    qreal scaleWidth = this->rect().width() / width;
    qreal scaleHeight = this->rect().height() / height;
    qreal realPixmapWidth = 0;
    qreal realPixmapHeight = 0;

    if(pixmapScale < screenScale){
        //图片比例小于屏幕比例时，按照图片和屏幕高度比进行缩放
        realPixmapWidth = width * scaleHeight;
        realPixmapHeight = this->rect().height();
    }else{
        //图片比例大于屏幕比例时，按照图片与屏幕宽度比进行缩放
        realPixmapWidth = this->rect().width();
        realPixmapHeight = height * scaleWidth;
    }

    QSize sourceSize = this->size();
    qint32 offsetX = 0;
    qint32 offsetY = 0;
    if (this->rect().width() == realPixmapWidth) {
        offsetY = (this->rect().height() - realPixmapHeight) / 2;
        sourceSize.setHeight(realPixmapHeight);
    } else if (this->rect().height() == realPixmapHeight) {
        offsetX = (this->rect().width() - realPixmapWidth) / 2;
        sourceSize.setWidth(realPixmapWidth);
    }

    // 规避xcb下闪线的问题
    sourceSize = sourceSize - QSize(1, 1);

    qDebug() << "=========getDestRect sourceSize:" << sourceSize;
    QPoint offsetPoint = this->rect().topLeft();
    offsetPoint += QPoint(offsetX, offsetY);

    return QRect(offsetPoint, sourceSize);
}

Peony::DesktopIconView *DesktopBackgroundWindow::getIconView()
{
    return m_desktopIconView;
}
