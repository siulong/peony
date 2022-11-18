#ifndef DIRECTORYVIEWHELPER_H
#define DIRECTORYVIEWHELPER_H

#include <QMap>
#include <QObject>
#include "peony-core_global.h"
#include "directory-view-widget.h"

namespace Peony {

class PEONYCORESHARED_EXPORT DirectoryViewIface2
{
public:
    explicit DirectoryViewIface2() {}
    virtual ~DirectoryViewIface2() {}

    virtual void doMultiSelect(bool) = 0;
    virtual bool isEnableMultiSelect() = 0;
};

class PEONYCORESHARED_EXPORT DirectoryViewHelper : public QObject
{
    Q_OBJECT
public:
    static DirectoryViewHelper *globalInstance();
    void addIconViewWithDirectoryViewWidget(DirectoryViewIface2 *iconView, DirectoryViewWidget *widget);
    void addListViewWithDirectoryViewWidget(DirectoryViewIface2 *listView, DirectoryViewWidget *widget);
    DirectoryViewIface2 *getViewIface2ByDirectoryViewWidget(DirectoryViewWidget *widget);

private:
    explicit DirectoryViewHelper(QObject *parent = nullptr);

    QMap<DirectoryViewWidget *, DirectoryViewIface2 *> m_views;
};

}

#endif // DIRECTORYVIEWHELPER_H
