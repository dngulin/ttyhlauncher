#include "downloadinfo.h"

const char* Ttyh::Json::DownloadInfo::keyUrl = "url";
const char* Ttyh::Json::DownloadInfo::keySha1 = "sha1";
const char* Ttyh::Json::DownloadInfo::keySize = "size";

Ttyh::Json::DownloadInfo::DownloadInfo(const QJsonObject &jObject)
{
    url = jObject[keyUrl].toString();
    sha1 = jObject[keySha1].toString();
    size = jObject[keySize].toInt();
}
