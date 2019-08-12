#ifndef LOGINREPLYDATA_H
#define LOGINREPLYDATA_H

#include <QtCore/QString>
#include <QtCore/QJsonObject>

#include "replydata.h"

namespace Ttyh {
namespace Master {
class LoginReplyData : public ReplyData
{
public:
    explicit LoginReplyData(const QJsonObject &jObject)
        : ReplyData(jObject),
          accessToken(jObject["accessToken"].toString("")),
          clientToken(jObject["clientToken"].toString("")) {};

    const QString accessToken;
    const QString clientToken;

    bool isValid() const { return !accessToken.isEmpty() && !clientToken.isEmpty(); }
};
}
}

#endif // LOGINREPLYDATA_H
