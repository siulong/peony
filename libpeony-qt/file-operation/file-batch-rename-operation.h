#ifndef FILEBATCHRENAMEOPERATION_H
#define FILEBATCHRENAMEOPERATION_H

#include "peony-core_global.h"
#include "file-operation.h"

namespace Peony {

class PEONYCORESHARED_EXPORT FileBatchRenameOperation : public FileOperation
{
    Q_OBJECT
public:
    explicit FileBatchRenameOperation(QStringList uris, QString newName);
    ~FileBatchRenameOperation();

    void run() override;
    std::shared_ptr<FileOperationInfo> getOperationInfo() override {
        return m_info;
    }

private:
    GFileCopyFlags m_default_copy_flag = GFileCopyFlags(G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA);
    QStringList m_uris ;
    QString m_new_name = nullptr;
    QStringList m_old_names;
    QStringList m_new_names;
    std::shared_ptr<FileOperationInfo> m_info = nullptr;

    ExceptionResponse m_apply_all = Other;

    QString getFileExtensionOfFile(const QString& file); /* 获取文件的文件扩展名 */
    ExceptionResponse prehandle(GError *err);
    QString handleDuplicate(const QString uri);


};

}
#endif // FILEBATCHRENAMEOPERATION_H
