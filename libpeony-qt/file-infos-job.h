#ifndef FILEINFOSJOB_H
#define FILEINFOSJOB_H

#endif // FILEINFOSJOB_H
#include "peony-core_global.h"

#include <QObject>

#include <memory>
#include <gio/gio.h>

namespace Peony {

class FileInfo;

class PEONYCORESHARED_EXPORT FileInfosJob : public QObject
{
    Q_OBJECT
public:
    explicit FileInfosJob(std::vector<std::shared_ptr<FileInfo> >infos, QObject *parent = nullptr);
    ~FileInfosJob();
    std::vector<std::shared_ptr<FileInfo> > batchQuerySync();

public Q_SLOTS:
    void batchCancel();/* 批量取消查询时使用 */

private:
    std::shared_ptr<FileInfo> queryFileType(std::shared_ptr<FileInfo>info, GFileInfo* new_info);
    std::shared_ptr<FileInfo> queryFileDisplayName(std::shared_ptr<FileInfo>info, GFileInfo* new_info);
    std::shared_ptr<FileInfo> refreshFileSystemInfo(std::shared_ptr<FileInfo>info, GFileInfo* new_info);
    std::shared_ptr<FileInfo> refreshInfoContents(std::shared_ptr<FileInfo>info, GFileInfo *new_info);

private:
    GCancellable *m_batchCanellable = nullptr;
    std::vector<std::shared_ptr<FileInfo> >m_infos;
};

}
