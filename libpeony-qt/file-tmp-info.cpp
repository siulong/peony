#include "file-tmp-info.h"

using namespace Peony;

FileTmpInfo::FileTmpInfo(QObject *parent) : QObject(parent)
{

}

FileTmpInfo::FileTmpInfo(const FileTmpInfo &fileInfo)
{
    m_uri = fileInfo.m_uri;
    m_is_dir = fileInfo.m_is_dir;
    m_is_virtual = fileInfo.m_is_virtual;
    m_display_name = fileInfo.m_display_name;
    m_icon_name = fileInfo.m_icon_name;
    m_path = fileInfo.m_path;
    m_contentType = fileInfo.m_contentType;
    m_size = fileInfo.m_size;
    m_modified_time = fileInfo.m_modified_time;
    m_access_time = fileInfo.m_access_time;
    m_deletion_data = fileInfo.m_deletion_data;
    m_can_read = fileInfo.m_can_read;
    m_can_write = fileInfo.m_can_write;
    m_can_excute = fileInfo.m_can_excute;
    m_can_delete = fileInfo.m_can_delete;
    m_can_trash = fileInfo.m_can_delete;
    m_can_rename = fileInfo.m_can_rename;
}

FileTmpInfo &FileTmpInfo::operator =(const FileTmpInfo &fileInfo)
{
    if (this == &fileInfo) {
        return *this;
    }

    m_uri = fileInfo.m_uri;
    m_is_dir = fileInfo.m_is_dir;
    m_is_virtual = fileInfo.m_is_virtual;
    m_display_name = fileInfo.m_display_name;
    m_icon_name = fileInfo.m_icon_name;
    m_path = fileInfo.m_path;
    m_contentType = fileInfo.m_contentType;
    m_size = fileInfo.m_size;
    m_modified_time = fileInfo.m_modified_time;
    m_access_time = fileInfo.m_access_time;
    m_deletion_data = fileInfo.m_deletion_data;
    m_can_read = fileInfo.m_can_read;
    m_can_write = fileInfo.m_can_write;
    m_can_excute = fileInfo.m_can_excute;
    m_can_delete = fileInfo.m_can_delete;
    m_can_trash = fileInfo.m_can_delete;
    m_can_rename = fileInfo.m_can_rename;
    return *this;
}

FileTmpInfo::~FileTmpInfo()
{

}

QString FileTmpInfo::targetUri() const
{
    return m_target_uri;
}

void FileTmpInfo::setTargetUri(const QString &targetUri)
{
    m_target_uri = targetUri;
}

bool FileTmpInfo::isDir() const
{
    return m_is_dir;
}

void FileTmpInfo::setDir(const bool &dir)
{
    m_is_dir = dir;
}

bool FileTmpInfo::isVirtual() const
{
    return m_is_virtual;
}

void FileTmpInfo::setVirtual(const bool &isVirtual)
{
    m_is_virtual = isVirtual;
}

QString FileTmpInfo::iconName() const
{
    return m_icon_name;
}

void FileTmpInfo::setIconName(const QString &iconName)
{
    m_icon_name = iconName;
}

QString FileTmpInfo::filePath() const
{
    return m_path;
}

void FileTmpInfo::setFilePath(const QString &path)
{
    m_path = path;
}

QString FileTmpInfo::contentType() const
{
    return m_contentType;
}

void FileTmpInfo::setContentType(const QString &contentType)
{
    m_contentType = contentType;
}

quint64 FileTmpInfo::size() const
{
    return m_size;
}

void FileTmpInfo::setSize(const quint64 &size)
{
    m_size = size;
}

QString FileTmpInfo::modifiedTime() const
{
    return m_modified_time;
}

void FileTmpInfo::setModifiedTime(const QString &modifiedTime)
{
    m_modified_time = modifiedTime;
}

QString FileTmpInfo::accessTime() const
{
    return m_access_time;
}

void FileTmpInfo::setAccessTime(const QString &accessTime)
{
    m_access_time = accessTime;
}

QString FileTmpInfo::deletionTime() const
{
    return m_deletion_data;
}

void FileTmpInfo::setDeletionTime(const QString &deletionTime)
{
    m_deletion_data = deletionTime;
}

bool FileTmpInfo::canRead() const
{
    return m_can_read;
}

void FileTmpInfo::setCanRead(const bool &canRead)
{
    m_can_read = canRead;
}

bool FileTmpInfo::canWrite() const
{
    return m_can_write;
}

void FileTmpInfo::setCanWrite(const bool &canWrite)
{
    m_can_write = canWrite;
}

bool FileTmpInfo::canExecute() const
{
    return m_can_excute;
}

void FileTmpInfo::setCanExecute(const bool &canExecute)
{
    m_can_excute = canExecute;
}

bool FileTmpInfo::canDelete() const
{
    return m_can_delete;
}

void FileTmpInfo::setCanDelete(const bool &canDelete)
{
    m_can_delete = canDelete;
}

bool FileTmpInfo::canTrash() const
{
    return m_can_trash;
}

void FileTmpInfo::setCanTrash(const bool &canTrash)
{
    m_can_trash = canTrash;
}

bool FileTmpInfo::canRename() const
{
    return m_can_rename;
}

void FileTmpInfo::setCanRename(const bool &canRename)
{
    m_can_rename = canRename;
}

void FileTmpInfo::addExtendInfo(const QString &key, const QVariant &value)
{
    m_extend_info.insert(key, value);
}

void FileTmpInfo::removeExtendInfo(const QString &key)
{
    m_extend_info.remove(key);
}

QVariant FileTmpInfo::getExtendInfo(const QString &key) const
{
    if (key.isEmpty()) {
        return QVariant();
    } else {
        return m_extend_info.value(key);
    }
}

QMap<QString, QVariant> FileTmpInfo::getAllExtendInfo() const
{
    return m_extend_info;
}
