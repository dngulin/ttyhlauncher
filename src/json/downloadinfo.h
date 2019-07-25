#ifndef DOWNLOADINFO_H
#define DOWNLOADINFO_H


#include <QtCore/QJsonObject>

namespace Ttyh
{
namespace Json
{
class DownloadInfo
{
public:
    explicit DownloadInfo(const QJsonObject &jObject);

    QString url;
    QString sha1;
    int size;
};
}
}


#endif //DOWNLOADINFO_H
