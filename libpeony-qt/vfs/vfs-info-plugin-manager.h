#ifndef VFSINFOPLUGINMANAGER_H
#define VFSINFOPLUGINMANAGER_H

#include "vfs-info-plugin-iface.h"

#include <gio/gio.h>

#include <QObject>
#include <QMap>

namespace Peony {
class PEONYCORESHARED_EXPORT VFSInfoPluginManager : public QObject
{
    Q_OBJECT
public:
    static VFSInfoPluginManager *getInstance();

    void registerPlugin(VFSInfoPluginIface *plugin);

    VFSInfoPluginIface* userSchemeGetPlugin(QString &scheme);

    const QStringList getAllPluginKeys();

private:
    explicit VFSInfoPluginManager(QObject *parent = nullptr);

    QMap<QString, VFSInfoPluginIface *> m_pluginsMap;
    QStringList m_support_path;
};

}


#endif // VFSINFOPLUGINMANAGER_H
