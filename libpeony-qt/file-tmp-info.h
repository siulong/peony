#ifndef FILETMPINFO_H
#define FILETMPINFO_H

#include "peony-core_global.h"

#include <gio/gio.h>
#include <QObject>
#include <QMap>
#include <QVariant>

namespace Peony {

// 是否需要更改更合适的名称
class  PEONYCORESHARED_EXPORT FileTmpInfo : public QObject
{
    Q_OBJECT
public:
    explicit FileTmpInfo(QObject *parent = nullptr);
    FileTmpInfo(const FileTmpInfo &fileInfo);

    FileTmpInfo &operator =(const FileTmpInfo &fileInfo);

    ~FileTmpInfo();

    QString targetUri() const;
    void setTargetUri(const QString &targetUri);

    bool isDir() const;
    void setDir(const bool &dir);

    bool isVirtual() const;
    void setVirtual(const bool &isVirtual);

    QString iconName() const;
    void setIconName(const QString &iconName);

    QString filePath() const;
    void setFilePath(const QString &path);

    QString contentType() const;
    void setContentType(const QString &contentType);

    quint64 size() const;
    void setSize(const quint64 &size);

    QString modifiedTime() const;
    void setModifiedTime(const QString &modifiedTime);

    QString accessTime() const;
    void setAccessTime(const QString &accessTime);

    QString deletionTime() const;
    void setDeletionTime(const QString &deletionTime);

    bool canRead() const;
    void setCanRead(const bool &canRead);

    bool canWrite() const;
    void setCanWrite(const bool &canWrite);

    bool canExecute() const;
    void setCanExecute(const bool &canExecute);

    bool canDelete() const;
    void setCanDelete(const bool &canDelete);

    bool canTrash() const;
    void setCanTrash(const bool &canTrash);

    bool canRename() const;
    void setCanRename(const bool &canRename);

    void addExtendInfo(const QString &key, const QVariant &value);
    void removeExtendInfo(const QString &key);
    QVariant getExtendInfo(const QString &key) const;
    QMap<QString, QVariant> getAllExtendInfo() const;

private:
    // todo: d-ptr
    QString m_uri = nullptr;
    QString m_target_uri = nullptr;
    bool m_is_dir = false;
    bool m_is_virtual = false;

    QString m_display_name = nullptr;
    QString m_icon_name = nullptr;
    QString m_path = nullptr;
    QString m_contentType = nullptr;
    guint64 m_size = 0;
    QString m_modified_time = nullptr;
    QString m_access_time = nullptr;
    QString m_deletion_data = nullptr;

    bool m_can_read = true;
    bool m_can_write = false;
    bool m_can_excute = false;
    bool m_can_delete = false;
    bool m_can_trash = false;
    bool m_can_rename = false;

    QMap<QString, QVariant> m_extend_info;
};

}


#endif // FILETMPINFO_H
