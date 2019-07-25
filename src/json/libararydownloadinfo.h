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

    int totalSize;

protected:
    static const char* keyTotalSize;
};
}
}


#endif //LIBARARYDOWNLOADINFO_H
