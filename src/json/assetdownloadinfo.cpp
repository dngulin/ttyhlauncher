#include "assetdownloadinfo.h"

Ttyh::Json::AssetDownloadInfo::AssetDownloadInfo(const QJsonObject &jObject)
    : DownloadInfo(jObject)
{
    id = jObject[keyId].toString();
    totalSize = jObject[keyTotalSize].toInt();
}

QJsonObject Ttyh::Json::AssetDownloadInfo::toJObject() const
{
    auto jObject = DownloadInfo::toJObject();

    jObject.insert(keyId, id);
    jObject.insert(keyTotalSize, totalSize);

    return jObject;
}
