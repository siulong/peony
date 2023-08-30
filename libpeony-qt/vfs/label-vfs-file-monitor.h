#ifndef LABELVFSFILEMONITOR_H
#define LABELVFSFILEMONITOR_H

#include <gio/gio.h>
#include <QObject>


G_BEGIN_DECLS

#define VFS_LABEL_FILE_MONITOR_NAME         ("vfs-label-file-monitor")

#define VFS_TYPE_LABEL_FILE_MONITOR         (vfs_label_file_monitor_get_type ())
#define VFS_LABEL_FILE_MONITOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), VFS_TYPE_LABEL_FILE_MONITOR, LabelVFSFileMonitor))
#define VFS_LABEL_FILE_MONITOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), VFS_TYPE_LABEL_FILE_MONITOR, LabelVFSFileMonitorClass))
#define VFS_IS_LABEL_FILE_MONITOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), VFS_TYPE_LABEL_FILE_MONITOR))
#define VFS_IS_LABEL_FILE_MONITOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), VFS_TYPE_LABEL_FILE_MONITOR))
#define VFS_LABEL_FILE_MONITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), VFS_TYPE_LABEL_FILE_MONITOR, LabelVFSFileMonitorClass))

/**
 * GFileMonitor:
 *
 * Watches for changes to a file.
 **/

struct LabelVFSFileMonitorPrivate
{
    //GList* fileList = nullptr;
    gchar*                       label_vfs_directory_uri;
};

struct LabelVFSFileMonitor
{
    GFileMonitor                 parent_instance;
    LabelVFSFileMonitorPrivate   *priv = nullptr;
    GList                        *fileList = nullptr;
    QMetaObject::Connection      add;
    QMetaObject::Connection      remove;
};

struct LabelVFSFileMonitorClass
{
    GFileMonitorClass  parent_class;
};


GType  vfs_label_file_monitor_get_type  (void) G_GNUC_CONST;
void   vfs_label_file_monitor_dir(LabelVFSFileMonitor *obj, const QString &label_vfs_directory_uri);


G_END_DECLS

#endif // LABELVFSFILEMONITOR_H
