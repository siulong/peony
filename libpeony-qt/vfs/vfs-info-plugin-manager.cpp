#include "vfs-info-plugin-manager.h"
#include "test-vfs-register.h"
#include <QDebug>

using namespace Peony;

static VFSInfoPluginManager *global_instance = nullptr;

VFSInfoPluginManager *Peony::VFSInfoPluginManager::getInstance()
{
    if (!global_instance) {
        global_instance = new VFSInfoPluginManager;
    }
    return global_instance;
}

void VFSInfoPluginManager::registerPlugin(Peony::VFSInfoPluginIface *plugin)
{
    if (m_support_path.contains(plugin->pathScheme())) {
        return;
    }

    m_pluginsMap.insert(plugin->pathScheme(), plugin);
    m_support_path.append(plugin->pathScheme());
    qDebug() << "============" << m_support_path;
}

VFSInfoPluginIface *VFSInfoPluginManager::userSchemeGetPlugin(QString &scheme)
{
    if (m_support_path.contains(scheme)) {
        return m_pluginsMap.value(scheme);
    }
    return nullptr;
}

const QStringList VFSInfoPluginManager::getAllPluginKeys()
{
    return m_pluginsMap.keys();
}

VFSInfoPluginManager::VFSInfoPluginManager(QObject *parent)
    : QObject(parent)
{
#ifndef VFS_CUSTOM_PLUGIN
    auto custom = new CustomVFSInfoInernalPlugin;
    registerPlugin(custom);

    auto local2 = new LocalVFSInfoInternalPlugin2;
    registerPlugin(local2);
#endif
}
