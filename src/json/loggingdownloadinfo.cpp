#include "loggingdownloadinfo.h"

Ttyh::Json::LoggingDownloadInfo::LoggingDownloadInfo(const QJsonObject &jObject)
    : DownloadInfo(jObject)
{
    id = jObject[keyId].toString();
}

QJsonObject Ttyh::Json::LoggingDownloadInfo::toJObject() const
{
    auto jObject = DownloadInfo::toJObject();
    jObject.insert(keyId, id);
    return jObject;
}
