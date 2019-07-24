#include "downloadinfo.h"

Ttyh::Json::DownloadInfo::DownloadInfo(const QJsonObject &jObject)
{
    url = jObject[keyUrl].toString();
    sha1 = jObject[keySha1].toString();
    size = jObject[keySize].toInt();
}

QJsonObject Ttyh::Json::DownloadInfo::toJObject() const
{
    return QJsonObject{
        {keyUrl, url},
        {keySha1, sha1},
        {keySize, size},
    };
}
