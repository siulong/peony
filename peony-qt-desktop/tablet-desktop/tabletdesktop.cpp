#include "tabletdesktop.h"
#include <QQuickItem>

TabletDesktop::TabletDesktop(QWidget *parent)
    : QQuickWidget(parent)
{
    setProperty("useWindowManagerStyle", false);
    setSource(QUrl(kQmlUrl));
}

void TabletDesktop::setGeometry(const QRect &rect)
{
    auto rootItem = this->rootObject();
    if (rootItem) {
        rootItem->setWidth(rect.width());
        rootItem->setHeight(rect.height());
    }
    return QQuickWidget::setGeometry(rect);
}

void TabletDesktop::showTabletDesktop()
{
    auto rootItem = this->rootObject();
    if (rootItem) {
        rootItem->setState("Show");
    }
}

void TabletDesktop::hideTabletDesktop()
{
    auto rootItem = this->rootObject();
    if (rootItem) {
        rootItem->setState("Hide");
    }
}
