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
    virtual QJsonObject toJObject() const;

    QString url;
    QString sha1;
    int size;

protected:
    const QString keyUrl = "url";
    const QString keySha1 = "sha1";
    const QString keySize = "size";

};
}
}


#endif //DOWNLOADINFO_H
