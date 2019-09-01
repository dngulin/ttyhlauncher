#include "libraryrule.h"
Ttyh::Json::Rule::Rule(const QJsonObject &jObject)
    : osName(jObject["os"].toObject()["name"].toString("")),
      osVersion(jObject["os"].toObject()["version"].toString("")),
      osArch(jObject["os"].toObject()["arch"].toString("")),
      action(jObject["action"].toString(""))
{
}
