#ifndef ASSETDOWNLOADINFO_H
#define ASSETDOWNLOADINFO_H


#include <QJsonObject>
#include "downloadinfo.h"

namespace Ttyh
{
namespace Json
{
class AssetDownloadInfo: public DownloadInfo
{
public:
    explicit AssetDownloadInfo(const QJsonObject &jObject);
    QJsonObject toJObject() const override;

    QString id;
    int totalSize;

protected:
    const QString keyId = "id";
    const QString keyTotalSize = "totalSize";
};
}
}


#endif //ASSETDOWNLOADINFO_H
