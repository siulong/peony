#ifndef TESTVFSFILE_H
#define TESTVFSFILE_H

#include <glib-object.h>
#include <gio/gio.h>
#include <QString>

G_BEGIN_DECLS


#define VFS_TYPE_TEST_FILE                      vfs_test_file_get_type()

#define VFS_IS_TEST_FILE_CLASS(k)               (G_TYPE_CHECK_CLASS_TYPE((k), VFS_TYPE_TEST_FILE))
#define VFS_IS_TEST_FILE(o)                     (G_TYPE_CHECK_INSTANCE_TYPE((o), VFS_TYPE_TEST_FILE))
#define VFS_TEST_FILE_CLASS(k)                  (G_TYPE_CLASS_CAST((k), VFS_TYPE_TEST_FILE, TestVFSFileClass))
#define VFS_TEST_FILE(o)                        (G_TYPE_CHECK_INSTANCE_CAST((o), VFS_TYPE_TEST_FILE, TestVFSFile))
#define VFS_TEST_FILE_GET_CLASS(o)              (G_TYPE_INSTANCE_GET_CLASS((o), VFS_TYPE_TEST_FILE, TestVFSFileClass))

G_DECLARE_FINAL_TYPE(TestVFSFile, vfs_test_file,
                     VFS, TEST_VFS, GObject)

TestVFSFile *vfs_test_file_new(void);

typedef struct
{
    gchar *uri;
    GFileMonitor* dirMonitor;
}TestVFSFilePrivate;

struct _TestVFSFile
{
    GObject parent_instance;
    TestVFSFilePrivate *priv;
};

G_END_DECLS

extern "C" {
    GFile *vfs_test_file_new_for_uri(const char *uri);
    static GFileEnumerator *vfs_test_file_enumerate_children_internal(GFile *file,
                                                                      const char *attribute,
                                                                      GFileQueryInfoFlags flags,
                                                                      GCancellable *cancellable,
                                                                      GError **error);
}

#endif // TESTVFSFILE_H
