#include "libraryrule.h"
Ttyh::Json::LibraryRule::LibraryRule(const QJsonObject &jObject)
{
    os = jObject["os"]["name"].toString();
    action = jObject["action"].toString();
}
