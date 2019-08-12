#ifndef REPLYDATA_H
#define REPLYDATA_H

#include <QtCore/QString>
#include <QtCore/QJsonObject>

namespace Ttyh {
namespace Master {
class ReplyData
{
public:
    explicit ReplyData(const QJsonObject &jObject)
        : error(jObject["error"].toString("")),
          errorMessage(jObject["errorMessage"].toString("")) {};

    const QString error;
    const QString errorMessage;
};
}
}

#endif // REPLYDATA_H
