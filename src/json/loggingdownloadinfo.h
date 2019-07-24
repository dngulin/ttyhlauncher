#ifndef LOGGINGDOWNLOADINFO_H
#define LOGGINGDOWNLOADINFO_H


#include <QJsonObject>
#include "downloadinfo.h"

namespace Ttyh
{
namespace Json
{
class LoggingDownloadInfo: public DownloadInfo
{
public:
    explicit LoggingDownloadInfo(const QJsonObject &jObject);
    QJsonObject toJObject() const override;

    QString id;

protected:
    const QString keyId = "id";
};
}
}


#endif //LOGGINGDOWNLOADINFO_H
