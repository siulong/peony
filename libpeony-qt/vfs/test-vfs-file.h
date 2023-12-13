/*
 * Peony-Qt's Library
 *
 * Copyright (C) 2023, KylinSoft Co., Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

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
