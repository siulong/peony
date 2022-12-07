#include "directoryviewhelper.h"



using namespace Peony;

static DirectoryViewHelper *global_instance = nullptr;

DirectoryViewHelper *DirectoryViewHelper::globalInstance()
{
    if (!global_instance) {
        global_instance = new DirectoryViewHelper;
    }
    return global_instance;
}

void DirectoryViewHelper::addIconViewWithDirectoryViewWidget(DirectoryViewIface2 *iconView, DirectoryViewWidget *widget)
{
    m_views.insert(widget, iconView);
}

void DirectoryViewHelper::addListViewWithDirectoryViewWidget(DirectoryViewIface2 *listView, DirectoryViewWidget *widget)
{
    m_views.insert(widget, listView);
}

DirectoryViewIface2 *DirectoryViewHelper::getViewIface2ByDirectoryViewWidget(DirectoryViewWidget *widget)
{
    return m_views.value(widget, nullptr);
}

DirectoryViewHelper::DirectoryViewHelper(QObject *parent) : QObject(parent)
{

}
