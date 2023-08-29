#include "label-vfs-register.h"
#include "label-vfs-file.h"
#include <gio/gio.h>
#include <QDebug>

bool Label_is_registed = false;

static GFile* test_vfs_parse_name (GVfs* vfs, const char* parse_name, gpointer user_data)
{
    return vfs_label_file_new_for_uri(parse_name);
}

static GFile* test_vfs_lookup (GVfs* vfs, const char *uri, gpointer user_data)
{
    return test_vfs_parse_name(vfs, uri, user_data);
}

void Peony::LabelVFSInternalPlugin::initVFS()
{
    LabelVFSRegister::registLabelVFS();
}

void *Peony::LabelVFSInternalPlugin::parseUriToVFSFile(const QString &uri)
{
    return vfs_label_file_new_for_uri(uri.toUtf8());
}

void Peony::LabelVFSRegister::registLabelVFS()
{
    if (Label_is_registed) {
        return;
    }

    GVfs* vfs = nullptr;
    gboolean res = false;
    const gchar * const *schemes;

    vfs = g_vfs_get_default ();
    schemes = g_vfs_get_supported_uri_schemes (vfs);

    const gchar* const* p = schemes;
    while (*p) {
        qDebug() << *p;
        p++;
    }

#if GLIB_CHECK_VERSION(2, 50, 0)
    res = g_vfs_register_uri_scheme (vfs, "label", test_vfs_lookup, NULL, NULL, test_vfs_parse_name, NULL, NULL);
#else
#endif
}

Peony::LabelVFSRegister::LabelVFSRegister()
{

}
