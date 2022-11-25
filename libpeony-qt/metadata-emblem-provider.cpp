#include "metadata-emblem-provider.h"
#include "file-meta-info.h"

using namespace Peony;

static MetadataEmblemProvider *global_instance = nullptr;

MetadataEmblemProvider *MetadataEmblemProvider::getInstance()
{
    if (!global_instance)
        global_instance = new MetadataEmblemProvider;
    return global_instance;
}

const QString MetadataEmblemProvider::emblemKey()
{
    return "metainfo-emblem";
}

QStringList MetadataEmblemProvider::getFileEmblemIcons(const QString &uri)
{
    auto metaInfo = FileMetaInfo::dupFromUri(uri);
    if(!metaInfo || !metaInfo.get())
        return QStringList();
    return metaInfo->getMetaInfoStringListV1("emblems");
}

MetadataEmblemProvider::MetadataEmblemProvider(QObject *parent) : EmblemProvider(parent)
{

}
