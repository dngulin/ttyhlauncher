#include "assetdownloadinfo.h"

Ttyh::Json::AssetDownloadInfo::AssetDownloadInfo(const QJsonObject &jObject)
    : DownloadInfo(jObject)
{
    id = jObject["id"].toString();
    totalSize = jObject["totalSize"].toInt();
}
