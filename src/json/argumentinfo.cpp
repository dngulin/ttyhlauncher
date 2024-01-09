#include <QtCore/QJsonArray>
#include "argumentinfo.h"

Ttyh::Json::ArgumentInfo::ArgumentInfo(const QJsonObject &jObject)
{
    for (const auto& jRule : jObject["rules"].toArray()) {
        rules << Rule(jRule.toObject());
    }

    auto jValue = jObject["value"];

    if (jValue.isString()) {
        values << jValue.toString();
    } else if (jValue.isArray()) {
        for (const auto& jValueEntry : jValue.toArray()) {
            values << jValueEntry.toString();
        }
    }
}

Ttyh::Json::ArgumentInfo::ArgumentInfo(const QString &arg)
{
    values << arg;
}
