#include "libararydownloadinfo.h"

const char* Ttyh::Json::LibraryDownloadInfo::keyTotalSize = "totalSize";

Ttyh::Json::LibraryDownloadInfo::LibraryDownloadInfo(const QJsonObject &jObject)
    : DownloadInfo(jObject)
{
    totalSize = jObject[keyTotalSize].toInt();
}
