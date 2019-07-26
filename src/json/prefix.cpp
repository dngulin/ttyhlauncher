#include "prefix.h"

Ttyh::Json::Prefix::Prefix()
{
    type = QString();
    about = QString();
}

Ttyh::Json::Prefix::Prefix(const QJsonObject &jObject)
{
    type = jObject[keyType].toString();
    about = jObject[keyAbout].toString();
}

QJsonObject Ttyh::Json::Prefix::toJsonObject() const
{
    return QJsonObject{
        {keyType, type},
        {keyAbout, about}
    };
}
