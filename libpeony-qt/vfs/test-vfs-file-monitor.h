#ifndef TESTVFSFILEMONITOR_H
#define TESTVFSFILEMONITOR_H

#include <gio/gio.h>
#include <QObject>

G_BEGIN_DECLS

#define VFS_TYPE_TEST_FILE_MONITOR (vfs_test_file_monitor_get_type())
#define VFS_TEST_FILE_MONITOR(o) (G_TYPE_CHECK_INSTANCE_CAST((o), VFS_TYPE_TEST_FILE_MONITOR, TestVFSFileMonitor))
#define VFS_TEST_FILE_MONITOR_CLASS(k) (G_TYPE_CLASS_CAST((k), VFS_TYPE_TEST_FILE_MONITOR, TestVFSFileMonitorClass))
#define VFS_IS_TEST_FILE_MONITOR(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), VFS_TYPE_TEST_FILE_MONITOR))
#define VFS_IS_TEST_FILE_MONITOR_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), VFS_TYPE_TEST_FILE_MONITOR))
#define VFS_TEST_FILE_MONITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS(o, VFS_TYPE_TEST_FILE_MONITOR, TestVFSFileMonitorClass))

typedef struct _TestVFSFileMonitor TestVFSFileMonitor;
typedef struct _TestVFSFileMonitorClass TestVFSFileMonitorClass;

struct _TestVFSFileMonitor
{
    GFileMonitor parent_monitor;
    QMetaObject::Connection add;
    QMetaObject::Connection remove;
    QMetaObject::Connection change;
};

struct _TestVFSFileMonitorClass {
    GFileMonitorClass parent_class;
};

GType vfs_test_file_monitor_get_type(void);
void vfs_test_file_monitor_dir(TestVFSFileMonitor *obj, const QString &filepath);

G_END_DECLS

#endif // TESTVFSFILEMONITOR_H
