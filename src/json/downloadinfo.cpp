#include "downloadinfo.h"

Ttyh::Json::DownloadInfo::DownloadInfo(const QJsonObject &jObject)
{
    url = jObject["url"].toString();
    sha1 = jObject["sha1"].toString();
    size = jObject["size"].toInt();
}
