#ifndef ASSETDOWNLOADINFO_H
#define ASSETDOWNLOADINFO_H

#include <QJsonObject>
#include "downloadinfo.h"

namespace Ttyh {
namespace Json {
class AssetDownloadInfo : public DownloadInfo
{
public:
    explicit AssetDownloadInfo(const QJsonObject &jObject);

    QString id;
    int totalSize;
};
}
}

#endif // ASSETDOWNLOADINFO_H
