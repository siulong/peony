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

#ifndef DESKTOPWINDOWMANAGER_H
#define DESKTOPWINDOWMANAGER_H

#include <QObject>
#include <QList>
#include <QScreen>
#include <QMap>
class DesktopBackgroundWindow;
class AdvancedDesktopIconView;
class QPoint;
namespace  Peony {
class AdvancedDesktopItemModel;
}
class DesktopWindowManager : public QObject
{
   Q_OBJECT
public:
    explicit DesktopWindowManager(QObject *parent = nullptr);
    ~DesktopWindowManager();

    static DesktopWindowManager *getInstance();
    QPoint postion(const QString &uri);
    QMap<QString, QPoint> getAllItemPos(const QStringList &uris);
    void createIdAndPos(int *id, QPoint *pos);
    void markCreateFilePos(const QPoint &pos);

public Q_SLOTS:
    void addBgWindow(QScreen *screen);
    void removeWindow(DesktopBackgroundWindow *window);

    void singleScreenMode();

    void multiscreenMode();

    void relocateIconView();

    DesktopBackgroundWindow *getIconView(QPoint pos);
    AdvancedDesktopIconView *getIconView(int id);
    DesktopBackgroundWindow *getIconView(QScreen *screen);

    void initModelFinish();
    int checkScreenMode(const QRect &geometry);
    void raiseWid();
    void layoutViewItems();
    void clearAllRestoreInfo();

private:
    int getDesktopWindowId();

Q_SIGNALS:
    void emitFinish();

private:
    QList<DesktopBackgroundWindow *> m_bgWindows;
    Peony::AdvancedDesktopItemModel *m_model = nullptr;
    QPoint m_markMousePressPos = QPoint(-1, -1);
    int m_markMousePressId = 0;
    int m_mode = 0;
    bool m_initMetaInfo = false;
    bool m_desktopAutoLayout = false;
};

#endif // DESKTOPWINDOWMANAGER_H
