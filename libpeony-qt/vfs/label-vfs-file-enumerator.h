#ifndef LABELVFSFILEENUMERATOR_H
#define LABELVFSFILEENUMERATOR_H

#include <gio/gio.h>
#include <QQueue>

G_BEGIN_DECLS

#define VFS_TYPE_LABEL_FILE_ENUMERATOR vfs_label_file_enumerator_get_type()

G_DECLARE_FINAL_TYPE(LabelVFSFileEnumerator, vfs_label_file_enumerator, VFS, LABEL_FILE_ENUMERATOR, GFileEnumerator)

LabelVFSFileEnumerator* vfs_label_file_enumerator_new(void);

struct LabelVFSFileEnumeratorPrivate
{
    QString*         label_vfs_directory_uri = nullptr;
    QQueue<QString>* enumerate_queue = nullptr;
};

struct _LabelVFSFileEnumerator
{
    GFileEnumerator parent_instance;

    LabelVFSFileEnumeratorPrivate*  priv = nullptr;
};

G_END_DECLS

#endif // LABELVFSFILEENUMERATOR_H
