#ifndef DESKTOPBACKGROUNDWINDOW_H
#define DESKTOPBACKGROUNDWINDOW_H

#include <QMainWindow>
#include <QGSettings>
#include "desktop-icon-view.h"
#include <KF5/KScreen/kscreen/output.h>

namespace KWayland {
namespace Client {
class PlasmaShellSurface;
}
}

class DesktopBackgroundWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit DesktopBackgroundWindow(const KScreen::OutputPtr &output, int desktopWindowId, QWidget *parent = nullptr);
    ~DesktopBackgroundWindow() override;

    int id() const;

    KScreen::OutputPtr screen() const;

    Peony::DesktopIconView *getIconView();
    void setId(int id);

    bool event(QEvent *event) override;

Q_SIGNALS:
    void setDefaultZoomLevel(Peony::DesktopIconView::ZoomLevel level);
    void setSortType(int sortType);
    void destroyed();

public Q_SLOTS:
    void setWindowGeometry(const QRect &geometry);
    void invaidScreen();

    void setCentralView();

protected Q_SLOTS:
    void updateWindowGeometry();

protected:
    void paintEvent(QPaintEvent *event) override;
    QPoint getRelativePos(const QPoint &pos);
    /**
     * 图片填充方式
     * @brief 从给定的图片中，截取一个与屏幕比例相同的矩形。(Rect,居中)。
     * @param pixmap 图片
     * @return
     */
    QRect getSourceRect(const QPixmap &pixmap);
    QRect getSourceRect(const QPixmap &pixmap, const QRect &screenGeometery);
    QRect getDestRect(const QPixmap &pixmap);

private:
    int m_id = -1;
    //QScreen *m_screen = nullptr;
    QGSettings *m_panelSetting = nullptr;
    Peony::DesktopIconView *m_desktopIconView = nullptr;

    KWayland::Client::PlasmaShellSurface *m_shellSurface = nullptr;
    KScreen::OutputPtr m_output = nullptr;
};

#endif // DESKTOPBACKGROUNDWINDOW_H
