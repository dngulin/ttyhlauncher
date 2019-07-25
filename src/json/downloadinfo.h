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

protected:
    static const char* keyUrl;
    static const char* keySha1;
    static const char* keySize;

};
}
}


#endif //DOWNLOADINFO_H
