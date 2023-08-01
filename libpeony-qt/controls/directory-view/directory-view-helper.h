#ifndef DIRECTORYVIEWHELPER_H
#define DIRECTORYVIEWHELPER_H

#include <QMap>
#include <QObject>
#include "peony-core_global.h"
#include "directory-view-widget.h"
#include "directory-view-plugin-iface.h"

namespace Peony {

class PEONYCORESHARED_EXPORT DirectoryViewHelper : public QObject
{
    Q_OBJECT
public:
    static DirectoryViewHelper *globalInstance();
    void addViewWithDirectoryViewWidget(DirectoryViewIface *view, DirectoryViewWidget *widget);
    DirectoryViewIface *getViewIfaceByDirectoryViewWidget(DirectoryViewWidget *widget);

Q_SIGNALS:
   void updateSelectStatus(bool status);

private:
    explicit DirectoryViewHelper(QObject *parent = nullptr);

    QMap<DirectoryViewWidget *, DirectoryViewIface *> m_views;
};

}

#endif // DIRECTORYVIEWHELPER_H
