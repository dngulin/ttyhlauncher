#ifndef LIBARARYDOWNLOADINFO_H
#define LIBARARYDOWNLOADINFO_H


#include <QJsonObject>
#include "downloadinfo.h"

namespace Ttyh
{
namespace Json
{
class LibraryDownloadInfo: public DownloadInfo
{
public:
    explicit LibraryDownloadInfo(const QJsonObject &jObject);
    QJsonObject toJObject() const override;

    int totalSize;

protected:
    const QString keyTotalSize = "totalSize";
};
}
}


#endif //LIBARARYDOWNLOADINFO_H
