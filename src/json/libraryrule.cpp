#include "libraryrule.h"
Ttyh::Json::LibraryRule::LibraryRule(const QJsonObject &jObject)
    : os(jObject["os"].toObject()["name"].toString("")), action(jObject["action"].toString(""))
{
}
