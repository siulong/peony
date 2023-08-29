#include "label-vfs-file-monitor.h"
#include "file-label-model.h"

G_DEFINE_TYPE (LabelVFSFileMonitor, vfs_label_file_monitor, G_TYPE_FILE_MONITOR)

static void vfs_label_file_monitor_dispose (GObject* obj);
static void vfs_label_file_monitor_finalize (GObject* obj);


static void vfs_label_file_monitor_init (LabelVFSFileMonitor* self)
{
    g_return_if_fail(VFS_IS_LABEL_FILE_MONITOR(self));
    self->fileList = nullptr;
}

static void vfs_label_file_monitor_class_init (LabelVFSFileMonitorClass* self)
{
    GObjectClass* selfClass = G_OBJECT_CLASS (self);
    selfClass->dispose = vfs_label_file_monitor_dispose;
    selfClass->finalize = vfs_label_file_monitor_finalize;
}

static void vfs_label_file_monitor_dispose (GObject* obj)
{
    g_return_if_fail(VFS_IS_LABEL_FILE_MONITOR(obj));
    LabelVFSFileMonitor* self = VFS_LABEL_FILE_MONITOR(obj);

    if (nullptr != self->fileList) {
        g_list_free_full(self->fileList, g_object_unref);
        self->fileList = nullptr;
    }

    QObject::disconnect(self->add);
    QObject::disconnect(self->remove);
}

static void vfs_label_file_monitor_finalize (GObject* obj)
{
    g_return_if_fail(VFS_IS_LABEL_FILE_MONITOR(obj));
    G_OBJECT_CLASS (vfs_label_file_monitor_parent_class)->finalize(obj);
}

static LabelVFSFileMonitor* vfs_label_file_monitor_new ()
{
    return VFS_LABEL_FILE_MONITOR(g_object_new (VFS_TYPE_LABEL_FILE_MONITOR, nullptr));
}

void vfs_label_file_monitor_free_gfile(LabelVFSFileMonitor* obj, GFile* file)
{
    LabelVFSFileMonitor* self = VFS_LABEL_FILE_MONITOR(obj);

    if (nullptr != file && G_IS_FILE(file)) {
        self->fileList = g_list_append(self->fileList, file);
    }
}

void vfs_label_file_monitor_dir(LabelVFSFileMonitor *obj, const QString &label_vfs_directory_uri)
{
    g_return_if_fail(VFS_IS_LABEL_FILE_MONITOR(obj));

    FileLabelModel* fileLabel = FileLabelModel::getGlobalModel();
    obj->add = QObject::connect(fileLabel, &FileLabelModel::fileLabelAdded, [=] (const QString &uri, bool successed) {
        if (successed && uri.startsWith(label_vfs_directory_uri)) {
            GFile* file = g_file_new_for_uri(uri.toUtf8().constData());
            g_file_monitor_emit_event(G_FILE_MONITOR(obj), file, nullptr, G_FILE_MONITOR_EVENT_CREATED);
            vfs_label_file_monitor_free_gfile (VFS_LABEL_FILE_MONITOR(obj), G_FILE(file));
        }
    });

    obj->remove = QObject::connect(fileLabel, &FileLabelModel::fileLabelRemoved, [=] (const QString &uri, bool successed) {
        if (successed && uri.startsWith(label_vfs_directory_uri)) {
            GFile* file = g_file_new_for_uri(uri.toUtf8().constData());
            g_file_monitor_emit_event(G_FILE_MONITOR(obj), file, nullptr, G_FILE_MONITOR_EVENT_DELETED);
            vfs_label_file_monitor_free_gfile (VFS_LABEL_FILE_MONITOR(obj), G_FILE(file));
        }
    });
}
