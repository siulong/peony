#include "test-vfs-file-monitor.h"
#include "test-vfs-file.h"
#include "vfs-info-plugin-iface.h"
#include "vfs-info-plugin-manager.h"
#include "file-utils.h"
#include <QDebug>

G_DEFINE_TYPE(TestVFSFileMonitor, vfs_test_file_monitor, G_TYPE_FILE_MONITOR)

static void vfs_test_file_monitor_dispose(GObject *obj);
static void vfs_test_file_monitor_finalize(GObject *obj);

static void vfs_test_file_monitor_init(TestVFSFileMonitor *self) {
    g_return_if_fail(VFS_IS_TEST_FILE_MONITOR(self));
}

static void vfs_test_file_monitor_class_init(TestVFSFileMonitorClass *klass) {
    GObjectClass* selfClass = G_OBJECT_CLASS(klass);

    selfClass->dispose = vfs_test_file_monitor_dispose;
    selfClass->finalize = vfs_test_file_monitor_finalize;
}

static void vfs_test_file_monitor_dispose(GObject *obj) {
    g_return_if_fail(VFS_IS_TEST_FILE_MONITOR(obj));
    TestVFSFileMonitor* self = VFS_TEST_FILE_MONITOR(obj);

    QObject::disconnect(self->add);
    QObject::disconnect(self->remove);
    QObject::disconnect(self->change);
}

static void vfs_test_file_monitor_finalize(GObject *obj) {
    g_return_if_fail(VFS_IS_TEST_FILE_MONITOR(obj));
    G_OBJECT_CLASS(vfs_test_file_monitor_parent_class)->finalize(obj);
}

static TestVFSFileMonitor *vfs_test_file_monitor_new() {
    return VFS_TEST_FILE_MONITOR(g_object_new(VFS_TYPE_TEST_FILE_MONITOR, nullptr));
}

void vfs_test_file_monitor_dir(TestVFSFileMonitor *obj, const QString &filepath) {
    g_return_if_fail(VFS_IS_TEST_FILE_MONITOR(obj));

    QString strUri = filepath;
    QString scheme = strUri.section(":", 0, -2);

    Peony::VFSInfoPluginIface *iface = Peony::VFSInfoPluginManager::getInstance()->userSchemeGetPlugin(scheme);
    Peony::HanderTransfer *transfer = new Peony::HanderTransfer();
    if (iface) {
        iface->handerMonitorDirectory(transfer, filepath);
    }

    obj->add = QObject::connect(transfer, &Peony::HanderTransfer::fileCreate, [=](const QString &path){
        qDebug() << "+++++++++++++" << path;
        if (!path.isEmpty()) {
            QString tScheme = scheme + "://";
            QString realUri = tScheme + path;
            g_autoptr(GFile) file = g_file_new_for_uri(realUri.toUtf8().constData());

            g_file_monitor_emit_event(G_FILE_MONITOR(obj), file, nullptr, G_FILE_MONITOR_EVENT_CREATED);
        }
    });

    obj->remove = QObject::connect(transfer, &Peony::HanderTransfer::fileDelete, [=](const QString &path){
        qDebug() << "+++++++++++++" << path;
        if (!path.isEmpty()) {
            QString tScheme = scheme + "://";
            QString realUri = tScheme + path;
            g_autoptr(GFile) file = g_file_new_for_uri(realUri.toUtf8().constData());

            g_file_monitor_emit_event(G_FILE_MONITOR(obj), file, nullptr, G_FILE_MONITOR_EVENT_DELETED);
        }
    });

    obj->change = QObject::connect(transfer, &Peony::HanderTransfer::fileChanged, [=](const QString &path){
        qDebug() << "+++++++++++++" << path;
        if (!path.isEmpty()) {
            QString tScheme = scheme + "://";
            QString realUri = tScheme + path;
            g_autoptr(GFile) file = g_file_new_for_uri(realUri.toUtf8().constData());

            g_file_monitor_emit_event(G_FILE_MONITOR(obj), file, nullptr, G_FILE_MONITOR_EVENT_CHANGED);
        }
    });
}
