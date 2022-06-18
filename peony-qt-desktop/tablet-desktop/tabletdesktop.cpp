#include "tabletdesktop.h"
#include <QQuickItem>

#include <QApplication>
#include <QScreen>

TabletDesktop::TabletDesktop(QWidget *parent)
    : QQuickWidget(parent)
{
    setProperty("useWindowManagerStyle", false);
    setSource(QUrl(kQmlUrl));

    this->connect(qApp, &QApplication::primaryScreenChanged, this, [=]{
        auto rect = qApp->primaryScreen()->geometry();
        rect.moveTo(0, 0);
        this->setGeometry(rect);
    });
    auto rect = qApp->primaryScreen()->geometry();
    rect.moveTo(0, 0);
    this->setGeometry(rect);
    this->showTabletDesktop();
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
