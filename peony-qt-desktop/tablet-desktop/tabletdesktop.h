#ifndef TABLETDESKTOP_H
#define TABLETDESKTOP_H

#include <QQuickWidget>

///
/// \brief The TabletDesktop class
/// 平板桌面类，使用方法:
/// TabletDesktop td;
/// td.setGeometry();
/// td.showTabletDesktop();
/// 硬件理想的情况下QQuikView的渲染性能更好，但是目前测试QQuickView
/// 在kubuntu wayland环境下无法支持触摸，暂时采用QQuickWidget的方案
///
class TabletDesktop : public QQuickWidget
{
    Q_OBJECT
public:
    explicit TabletDesktop(QWidget *parent = nullptr);

    ///
    /// \brief setGeometry 会同时设置TabletDesktop和TabletDesktop加载的qml对象的宽度和高度
    /// \param rect
    ///
    void setGeometry(const QRect &rect);
    inline void setGeometry(int x, int y, int w, int h)
    { this->setGeometry(QRect(x, y, w, h)); }

    ///
    /// \brief showTabletDesktop 显示平板桌面
    /// \note 不会影响TabletDesktop的显示和隐藏效果
    ///
    void showTabletDesktop();

    ///
    /// \brief hideTabletDesktop 隐藏平板桌面
    /// \note 不会影响TabletDesktop的显示和隐藏效果
    ///
    void hideTabletDesktop();

private:
    const QString kQmlUrl = "/usr/share/ukui/tablet/contents/ui/main.qml";
};

#endif // TABLETDESKTOP_H
