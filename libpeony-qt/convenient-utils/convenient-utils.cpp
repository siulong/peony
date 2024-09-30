#include "convenient-utils.h"
#include "file-enumerator.h"
#include "file-item-model.h"
#include "file-item-proxy-filter-sort-model.h"
#include "file-info-manager.h"
#include "file-info-job.h"
#include "file-infos-job.h"
#include "file-info.h"

#include <QObject>

using namespace Peony;

ConvenientUtils *ConvenientUtils::global_instance = nullptr;

ConvenientUtils *ConvenientUtils::getInstance()
{
    if (!global_instance) {
        global_instance = new ConvenientUtils;
    }
    return global_instance;
}

ConvenientUtils::ConvenientUtils(QObject *parent) : QObject(parent)
{

}

QStringList ConvenientUtils::getFileUrisInSequence(const QString &uri) const
{
    QStringList orderedFileUris;
    /* uri遍历方式获取子项 */
    FileEnumerator e;
    e.setEnumerateDirectory(uri);
    e.enumerateSync();
    std::vector<std::shared_ptr<FileInfo> > fileInfoVec;
    for (auto fileInfo : e.getChildren()) {
        FileInfoJob infoJob(fileInfo);
        infoJob.querySync();
        fileInfoVec.push_back(fileInfo);
    }

    FileItemModel *model = new FileItemModel();
    FileItemProxyFilterSortModel *proxy_model = new FileItemProxyFilterSortModel();
    proxy_model->setSourceModel(model);

    auto fileInfo = FileInfo::fromUri(uri);
    FileInfoJob infoJob(fileInfo);
    infoJob.querySync();
    auto item = new FileItem(fileInfo, nullptr, model, nullptr);

    model->insetFileInfoData(fileInfoVec, item);
    proxy_model->checkSettingsAndSort();
    orderedFileUris = proxy_model->getAllFileUris();

    if(model){
        delete model;
        model = nullptr;
    }
    if(proxy_model){
        delete proxy_model;
        proxy_model = nullptr;
    }
    return orderedFileUris;
}


