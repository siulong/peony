#ifndef LABELVFSFILE_H
#define LABELVFSFILE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define VFS_TYPE_LABEL_FILE                      (vfs_label_file_get_type())

#define VFS_IS_LABEL_FILE_CLASS(k)               (G_TYPE_CHECK_CLASS_TYPE((k), VFS_TYPE_LABEL_FILE))
#define VFS_IS_FAVORITE_FILE(o)                  (G_TYPE_CHECK_INSTANCE_TYPE((o), VFS_TYPE_LABEL_FILE))
#define VFS_LABEL_FILE_CLASS(k)                  (G_TYPE_CLASS_CAST((k), VFS_TYPE_LABEL_FILE, LabelVFSFileClass))
//#define VFS_LABEL_FILE(o)                        (G_TYPE_CHECK_INSTANCE_CAST((o), VFS_TYPE_LABEL_FILE, LabelVFSFile))
#define VFS_LABEL_FILE_GET_CLASS(o)              (G_TYPE_INSTANCE_GET_CLASS((o), VFS_TYPE_LABEL_FILE, LabelVFSFileClass))

G_DECLARE_FINAL_TYPE(LabelVFSFile, vfs_label_file, VFS, LABEL_FILE, GObject)

LabelVFSFile* vfs_label_file_new(void);

struct LabelVFSFilePrivate
{
    gchar*                      uri;
    GFileMonitor*               fileMonitor;
};

struct _LabelVFSFile
{
    GObject                     parent_instance;
    LabelVFSFilePrivate*        priv;
};

GFile*   vfs_label_file_new_for_uri(const char *uri);
gboolean vfs_label_file_is_exist (const char* uri);

G_END_DECLS

#endif // LABELVFSFILE_H
