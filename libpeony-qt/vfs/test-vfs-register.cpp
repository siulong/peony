#include "test-vfs-register.h"
#include "test-vfs-file.h"
#include "file-enumerator.h"
#include "file-info.h"
#include "file-info-job.h"
#include "file-utils.h"
#include "vfs-info-plugin-manager.h"
#include "global-settings.h"
#include <gio/gio.h>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QDebug>

#define CUSTPM_FILE_PATH "/home/xwj/peonyConfig.txt"
#define LOCAL_FILE_PATH "file:///home/xwj/hello"
#define LOCAL_FILE_PATH2 "file:///home/xwj/文档"

using namespace Peony;

bool test_is_registed = false;

static GFile *
test_vfs_parse_name (GVfs       *vfs,
                     const char *parse_name,
                     gpointer    user_data)
{
    Q_UNUSED(vfs);
    Q_UNUSED(user_data);
    return vfs_test_file_new_for_uri(parse_name);
}

static GFile *
test_vfs_lookup (GVfs       *vfs,
                 const char *uri,
                 gpointer    user_data)
{
    return test_vfs_parse_name(vfs, uri, user_data);
}

TestVFSInternalPlugin::TestVFSInternalPlugin(QString scheme)
    :m_scheme(scheme)
{
}

void TestVFSInternalPlugin::initVFS()
{
    if (!m_scheme.isEmpty()) {
        TestVFSRegister::registTestVFS(m_scheme);
    }
}

QString TestVFSInternalPlugin::uriScheme()
{
    QString uriScheme;
    if (!m_scheme.isEmpty()) {
        uriScheme = m_scheme + "://";
    }
    return uriScheme;
}

bool TestVFSInternalPlugin::holdInSideBar()
{
    bool isShow = true;
    if (!m_scheme.isEmpty()) {
        auto iface = VFSInfoPluginManager::getInstance()->userSchemeGetPlugin(m_scheme);
        if (iface) {
            isShow = iface->holdInSideBar();
        }
    }
    return isShow;
}

void *TestVFSInternalPlugin::parseUriToVFSFile(const QString &uri)
{
    return vfs_test_file_new_for_uri(uri.toUtf8().constData());
}

CustomErrorHandler *TestVFSInternalPlugin::customErrorHandler()
{
    if (!m_scheme.isEmpty()) {
        auto iface = VFSInfoPluginManager::getInstance()->userSchemeGetPlugin(m_scheme);
        if (iface) {
            return iface->customErrorHandler();
        }
    }
    return nullptr;
}

void TestVFSRegister::registTestVFS(QString scheme)
{
    if (test_is_registed) {
        return;
    }

    GVfs *vfs;
    const gchar * const *schemes;
    gboolean res = false;

    vfs = g_vfs_get_default();
    schemes = g_vfs_get_supported_uri_schemes(vfs);
    const gchar * const *p;
    p = schemes;
    while (*p) {
        qDebug() << *p;
        if (*p == scheme) {
            qDebug() << __func__ << *p << scheme;
            break;
        }
        p++;
    }

    if (scheme.isEmpty()) {
#if GLIB_CHECK_VERSION(2, 50, 0)
    res = g_vfs_register_uri_scheme(vfs, "test",
                                    test_vfs_lookup, NULL, NULL,
                                    test_vfs_parse_name, NULL, NULL);
#else
#endif
    } else {
#if GLIB_CHECK_VERSION(2, 50, 0)
    res = g_vfs_register_uri_scheme(vfs, scheme.toUtf8().constData(),
                                    test_vfs_lookup, NULL, NULL,
                                    test_vfs_parse_name, NULL, NULL);
#else
#endif
    }
}

TestVFSRegister::TestVFSRegister()
{

}

#ifndef VFS_CUSTOM_PLUGIN
CustomVFSInfoInernalPlugin::CustomVFSInfoInernalPlugin()
{
    QFile file(CUSTPM_FILE_PATH);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "open config file failed!" << file.error();
        return;
    }

    QTextStream textStream(&file);
    QString str;
    while (!textStream.atEnd()) {
        //str.append(textStream.readLine());
        str = textStream.readLine();
        if (str.startsWith("node:") && str.contains("parent:")) {
            QStringList list = str.split(',');
            QString first = list.at(0);
            QString second = list.at(1);
            QString fileName = first.remove("node:");
            QString dirPath = second.remove("parent:");

            QString filePath;
            if (dirPath == "/") {
                filePath =  QString("%1%2").arg(dirPath).arg(fileName);
            } else {
                filePath = QString("%1%2%3").arg(dirPath).arg("/").arg(fileName);
            }

            QStringList tmp = m_allInfoMap.value(dirPath);
            tmp.append(filePath);
            tmp.removeDuplicates();
            m_allInfoMap.insert(dirPath, tmp);
        }
    }
    file.close();
    qDebug() << __func__ << m_allInfoMap;
}

QString CustomVFSInfoInernalPlugin::pathScheme()
{
    return QString("custom");
}

QStringList CustomVFSInfoInernalPlugin::fileEnumerator(const QString &path)
{
    QStringList list;
    list = m_allInfoMap.value(path);
    return list;
}

std::shared_ptr<FileTmpInfo> CustomVFSInfoInernalPlugin::queryFile(const QString &path)
{
    bool isDir = false;
    if (m_allInfoMap.keys().contains(path)) {
        isDir = true;
    }

    QString uri = QString("test://%1").arg(path);

    qDebug() << __func__ << path << m_allInfoMap.keys() << uri << isDir;
    std::shared_ptr<FileTmpInfo> file = std::make_shared<FileTmpInfo>();
    file->setDir(isDir);
    return file;
}

VFSError* CustomVFSInfoInernalPlugin::handerMakeDirectory(const QString &uri, bool &ret)
{
    return nullptr;
}

VFSError *CustomVFSInfoInernalPlugin::handerFileCreate(const QString &uri, bool &ret)
{
    return nullptr;
}

VFSError *CustomVFSInfoInernalPlugin::handerFileDelete(const QString &uri, bool &ret)
{
    return nullptr;
}

VFSError *CustomVFSInfoInernalPlugin::handerFileCopy(const QString &sourceUri, const QString &destUri, bool &ret)
{
    return nullptr;
}

VFSError *CustomVFSInfoInernalPlugin::handerFileMove(const QString &sourceUri, const QString &destUri, bool &ret)
{
    return nullptr;
}

VFSError *CustomVFSInfoInernalPlugin::handerFileRename(const QString &sourceUri, const QString &newName)
{
    return nullptr;
}

void CustomVFSInfoInernalPlugin::handerMonitorDirectory(HanderTransfer *transfer, const QString &uri)
{
}

LocalVFSInfoInternalPlugin2::LocalVFSInfoInternalPlugin2()
{
}

QString LocalVFSInfoInternalPlugin2::pathScheme()
{
    return QString("local2");
}

QStringList LocalVFSInfoInternalPlugin2::fileEnumerator(const QString &path)
{
    QStringList list;
    QString dir = LOCAL_FILE_PATH2 + path;
    FileEnumerator e;
    e.setEnumerateDirectory(dir);
    e.enumerateSync();
    QStringList uris = e.getChildrenUris();
    for (auto uri : uris) {
        QString tmp = FileUtils::urlDecode(uri);
        tmp.remove(LOCAL_FILE_PATH2);
        list.append(tmp);
    }
    list.removeDuplicates();
    qDebug() << __func__ << list;

    return list;
}

std::shared_ptr<FileTmpInfo> LocalVFSInfoInternalPlugin2::queryFile(const QString &path)
{
    QString tPath = path;
    std::shared_ptr<FileTmpInfo> file = std::make_shared<FileTmpInfo>();

    QString uri = LOCAL_FILE_PATH2 + tPath;
    g_autoptr(GFile) tmp = g_file_new_for_uri(uri.toUtf8().constData());
    g_autoptr(GFileInfo) fileInfo = nullptr;
    if (tmp) {
        fileInfo = g_file_query_info(tmp, "*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, nullptr, nullptr);
        if (fileInfo) {
            GFileType fileType = g_file_info_get_file_type(fileInfo);
            QString contentType = g_file_info_get_content_type(fileInfo);
            if (fileType == G_FILE_TYPE_DIRECTORY || contentType == "inode/directory") {
                file->setDir(true);
            }

            guint64 modifiedTime = g_file_info_get_attribute_uint64(fileInfo, G_FILE_ATTRIBUTE_TIME_MODIFIED);
            guint64 accessTime = g_file_info_get_attribute_uint64(fileInfo, G_FILE_ATTRIBUTE_TIME_ACCESS);
            QDateTime modifiedDate = QDateTime::fromMSecsSinceEpoch(modifiedTime*1000);
            QDateTime accessDate = QDateTime::fromMSecsSinceEpoch(accessTime*1000);
            QString mtime = modifiedDate.toString(GlobalSettings::getInstance()->getSystemTimeFormat());
            QString atime = accessDate.toString(GlobalSettings::getInstance()->getSystemTimeFormat());
            QString iconName;

            GIcon *g_icon = g_file_info_get_icon(fileInfo);
            if (G_IS_THEMED_ICON(g_icon)) {
                const gchar* const* icon_names = g_themed_icon_get_names(G_THEMED_ICON (g_icon));
                if (icon_names) {
                    auto p = icon_names;
                    while (*p) {
                        QIcon icon = QIcon::fromTheme(*p);
                        if (!icon.isNull()) {
                            iconName = QString (*p);
                            break;
                        } else {
                            p++;
                        }
                    }
                }
            }

           bool has_unix_mode = g_file_info_has_attribute(fileInfo, G_FILE_ATTRIBUTE_UNIX_MODE);
            guint32 mode = 0;
            if (has_unix_mode)
                mode = g_file_info_get_attribute_uint32(fileInfo, G_FILE_ATTRIBUTE_UNIX_MODE);

            qDebug() << __func__ << mode;
            file->setTargetUri(uri);
            file->setIconName(iconName);
            file->setModifiedTime(mtime);
            file->setAccessTime(atime);
            file->addExtendInfo(G_FILE_ATTRIBUTE_UNIX_MODE, QVariant(mode));
            file->setSize(g_file_info_get_attribute_uint64(fileInfo, G_FILE_ATTRIBUTE_STANDARD_SIZE));
            file->setContentType(contentType);
        } else {
            qDebug() << __func__ << "is null";
            return nullptr;
        }
    }
    return file;
}

VFSError* LocalVFSInfoInternalPlugin2::handerMakeDirectory(const QString &uri, bool &ret)
{
    QString strUri = uri;
    QString realUri;
    QString scheme = pathScheme() + "://";
    struct VFSError* err = nullptr;

    strUri.remove(scheme);
    realUri = LOCAL_FILE_PATH2 + strUri;

    g_autoptr(GFile) destFile = g_file_new_for_uri(realUri.toUtf8().constData());
    g_autoptr(GError) error = nullptr;
    ret = g_file_make_directory(destFile, nullptr, &error);
    qDebug() << __func__ << uri << realUri << ret;
    if (error) {
        err = new VFSError;
        err->code = Peony::ErrorType(error->code);
        err->message = QString(error->message);
    }
    return err;
}

VFSError *LocalVFSInfoInternalPlugin2::handerFileCreate(const QString &uri, bool &ret)
{
    QString strUri = uri;
    QString realUri;
    QString scheme = pathScheme() + "://";
    struct VFSError* err = nullptr;

    strUri.remove(scheme);
    realUri = LOCAL_FILE_PATH2 + strUri;

    g_autoptr(GFile) destFile = g_file_new_for_uri(realUri.toUtf8().constData());
    g_autoptr(GError) error = nullptr;
    g_file_create(destFile, G_FILE_CREATE_NONE, nullptr, &error);
    qDebug() << __func__ << uri << realUri << ret;
    if (error) {
        ret = false;
        err = new VFSError;
        err->code = Peony::ErrorType(error->code);
        err->message = QString(error->message);
    } else {
        ret = true;
    }
    return err;
}

VFSError *LocalVFSInfoInternalPlugin2::handerFileDelete(const QString &uri, bool &ret)
{
    QString strUri = uri;
    QString realUri;
    QString scheme = pathScheme() + "://";
    struct VFSError* err = nullptr;

    strUri.remove(scheme);
    realUri = LOCAL_FILE_PATH2 + strUri;

    g_autoptr(GFile) destFile = g_file_new_for_uri(realUri.toUtf8().constData());
    g_autoptr(GError) error = nullptr;
    ret = g_file_delete(destFile, nullptr, &error);
    qDebug() << __func__ << uri << realUri << ret;
    if (error) {
        err = new VFSError;
        err->code = Peony::ErrorType(error->code);
        err->message = QString(error->message);
    } else {
        ret = true;
    }
    return err;
}

VFSError *LocalVFSInfoInternalPlugin2::handerFileCopy(const QString &sourceUri, const QString &destUri, bool &ret)
{
    QString strSourceFile = sourceUri;
    QString strDestFile = destUri;
    QString strSourceTrueUri = strSourceFile;
    QString strDestTrueUri = strDestFile;
    QString sourceScheme = strSourceFile.section(":", 0, -2);
    QString destScheme = strDestFile.section(":", 0, -2);
    struct VFSError* err = nullptr;

    if (strSourceFile.startsWith(pathScheme())) {
        sourceScheme += "://";
        strSourceTrueUri.remove(sourceScheme);
        strSourceTrueUri = LOCAL_FILE_PATH2 + strSourceTrueUri;
        qDebug() << "strSourceTrueUri" << __func__ << strSourceTrueUri;
    }

    if (strDestFile.startsWith(pathScheme())) {
        destScheme += "://";
        strDestTrueUri.remove(destScheme);
        strDestTrueUri = LOCAL_FILE_PATH2 + strDestTrueUri;
        qDebug() << "strDestTrueUri" << __func__ << strDestTrueUri;
    }

    GFile *gsrcFile = g_file_new_for_uri(strSourceTrueUri.toUtf8().constData());
    GFile *gdestFile = g_file_new_for_uri(strDestTrueUri.toUtf8().constData());
    GError *error = nullptr;

    ret = g_file_copy(gsrcFile, gdestFile, GFileCopyFlags(G_FILE_COPY_NONE|G_FILE_COPY_OVERWRITE), nullptr, nullptr, nullptr, &error);

    if (gsrcFile) {
        g_object_unref(gsrcFile);
    }

    if (gdestFile) {
        g_object_unref(gdestFile);
    }

    if (error) {
        err = new VFSError;
        err->code = ErrorType(error->code);
        err->message = QString(error->message);
        g_error_free(error);
    }

    return err;
}

VFSError *LocalVFSInfoInternalPlugin2::handerFileMove(const QString &sourceUri, const QString &destUri, bool &ret)
{
    QString strSourceFile = sourceUri;
    QString strDestFile = destUri;
    QString strSourceTrueUri = strSourceFile;
    QString strDestTrueUri = strDestFile;
    QString sourceScheme = strSourceFile.section(":", 0, -2);
    QString destScheme = strDestFile.section(":", 0, -2);
    struct VFSError* err = nullptr;

    if (strSourceFile.startsWith(pathScheme())) {
        sourceScheme += "://";
        strSourceTrueUri.remove(sourceScheme);
        strSourceTrueUri = LOCAL_FILE_PATH2 + strSourceTrueUri;
        qDebug() << "strSourceTrueUri" << __func__ << strSourceTrueUri;
    }

    if (strDestFile.startsWith(pathScheme())) {
        destScheme += "://";
        strDestTrueUri.remove(destScheme);
        strDestTrueUri = LOCAL_FILE_PATH2 + strDestTrueUri;
        qDebug() << "strDestTrueUri" << __func__ << strDestTrueUri;
    }

    GFile *gsrcFile = g_file_new_for_uri(strSourceTrueUri.toUtf8().constData());
    GFile *gdestFile = g_file_new_for_uri(strDestTrueUri.toUtf8().constData());
    GError *error = nullptr;

    ret = g_file_move(gsrcFile, gdestFile, GFileCopyFlags(G_FILE_COPY_NONE|G_FILE_COPY_ALL_METADATA), nullptr, nullptr, nullptr, &error);

    if (gsrcFile) {
        g_object_unref(gsrcFile);
    }

    if (gdestFile) {
        g_object_unref(gdestFile);
    }

    if (error) {
        err = new VFSError;
        err->code = ErrorType(error->code);
        err->message = QString(error->message);
        g_error_free(error);
    }

    return err;
}

VFSError *LocalVFSInfoInternalPlugin2::handerFileRename(const QString &sourceUri, const QString &newName)
{
    QString sUri = sourceUri;
    QString scheme = pathScheme() + "://";
    QString realUri;
    struct VFSError* err = nullptr;
    sUri.remove(scheme);
    sUri = LOCAL_FILE_PATH2 + sUri;

    qDebug() << __func__ << realUri;
    g_autoptr(GFile) tFile = nullptr;
    g_autoptr(GFile) retFile = nullptr;
    g_autoptr(GError) error = nullptr;

    tFile = g_file_new_for_uri(sUri.toUtf8().constData());
    retFile = g_file_set_display_name(tFile, newName.toUtf8().constData(), nullptr, &error);

    if (error) {
        err = new VFSError;
        err->code = Peony::ErrorType(error->code);
        err->message = QString(error->message);
    }

    return err;
}

void LocalVFSInfoInternalPlugin2::handerMonitorDirectory(HanderTransfer *transfer, const QString &uri)
{
    QString strUri = uri;
    QString scheme = pathScheme() + "://";
    QString realPath = LOCAL_FILE_PATH2 + strUri.remove(scheme);
    if (!realPath.isEmpty()) {
        qDebug() << __func__ << uri << realPath;
        FileWatcher *watcher = new FileWatcher(realPath);
        watcher->setMonitorChildrenChange(true);
        QObject::connect(watcher, &FileWatcher::fileCreated, [=](const QString &uri){
            QString strUri = Peony::FileUtils::urlDecode(uri);
            strUri.remove(LOCAL_FILE_PATH2);
            qDebug() << "fileCreated" << uri << strUri;
            Q_EMIT transfer->fileCreate(strUri);
        });
        QObject::connect(watcher, &FileWatcher::fileDeleted, [=](const QString &uri){
            QString strUri = Peony::FileUtils::urlDecode(uri);
            strUri.remove(LOCAL_FILE_PATH2);
            qDebug() << "fileDeleted" << uri << strUri;
           Q_EMIT transfer->fileDelete(strUri);
        });
        QObject::connect(watcher, &FileWatcher::fileChanged, [=](const QString &uri){
            QString strUri = Peony::FileUtils::urlDecode(uri);
            strUri.remove(LOCAL_FILE_PATH2);
            qDebug() << "fileChanged" << uri << strUri;
            Q_EMIT transfer->fileChanged(strUri);
        });
        watcher->startMonitor();
    }
}
#endif
