#include "libararydownloadinfo.h"

Ttyh::Json::LibraryDownloadInfo::LibraryDownloadInfo(const QJsonObject &jObject)
    : DownloadInfo(jObject)
{
    totalSize = jObject[keyTotalSize].toInt();
}

QJsonObject Ttyh::Json::LibraryDownloadInfo::toJObject() const
{
    auto jObject = DownloadInfo::toJObject();
    jObject.insert(keyTotalSize, totalSize);
    return jObject;
}
