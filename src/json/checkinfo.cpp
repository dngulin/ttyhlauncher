#include "checkinfo.h"

Ttyh::Json::CheckInfo::CheckInfo(const QJsonObject &jObject)
{
    hash = jObject[keyHash].toString();
    size = jObject[keySize].toInt();
}

QJsonObject Ttyh::Json::CheckInfo::toJObject() const
{
    return QJsonObject{
        {keyHash, hash},
        {keySize, size}
    };
}
