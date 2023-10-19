#ifndef TESTVFSFILEENUMERATOR_H
#define TESTVFSFILEENUMERATOR_H

#include <gio/gio.h>

#include <QQueue>

G_BEGIN_DECLS

#define VFS_TYPE_TEST_FILE_ENUMERATOR vfs_test_file_enumerator_get_type()

G_DECLARE_FINAL_TYPE(TestVFSFileEnumerator, vfs_test_file_enumerator, VFS, TEST_FILE_ENUMERATOR, GFileEnumerator)

TestVFSFileEnumerator* vfs_test_file_enumerator_new(void);

typedef struct _TestVFSFileEnumeratorPrivate       TestVFSFileEnumeratorPrivate;

struct _TestVFSFileEnumeratorPrivate
{
    QQueue<QString>*                    enumerate_queue;
};

struct _TestVFSFileEnumerator
{
    GFileEnumerator                     parent_instance;

    TestVFSFileEnumeratorPrivate*  priv;
};

G_END_DECLS

#endif // TESTVFSFILEENUMERATOR_H
