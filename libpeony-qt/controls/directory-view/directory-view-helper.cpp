#include "directory-view-helper.h"

using namespace Peony;

static DirectoryViewHelper *global_instance = nullptr;

DirectoryViewHelper *DirectoryViewHelper::globalInstance()
{
    if (!global_instance) {
        global_instance = new DirectoryViewHelper;
    }
    return global_instance;
}

void DirectoryViewHelper::addViewWithDirectoryViewWidget(DirectoryViewIface *view, DirectoryViewWidget *widget)
{
    m_views.insert(widget, view);
}

DirectoryViewIface *DirectoryViewHelper::getViewIfaceByDirectoryViewWidget(DirectoryViewWidget *widget)
{
    return m_views.value(widget, nullptr);
}

DirectoryViewHelper::DirectoryViewHelper(QObject *parent) : QObject(parent)
{

}
