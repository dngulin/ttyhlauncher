#include "assetdownloadinfo.h"

const char* Ttyh::Json::AssetDownloadInfo::keyId = "id";
const char* Ttyh::Json::AssetDownloadInfo::keyTotalSize = "totalSize";

Ttyh::Json::AssetDownloadInfo::AssetDownloadInfo(const QJsonObject &jObject)
    : DownloadInfo(jObject)
{
    id = jObject[keyId].toString();
    totalSize = jObject[keyTotalSize].toInt();
}
