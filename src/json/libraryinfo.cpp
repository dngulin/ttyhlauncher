#include <QtCore/QJsonArray>
#include "libraryinfo.h"

Ttyh::Json::LibraryInfo::LibraryInfo(const QJsonObject &jObject)
    : name(jObject["name"].toString(""))
{
    foreach (auto jRule, jObject["rules"].toArray()) {
        rules << Rule(jRule.toObject());
    }

    auto jNatives = jObject["natives"].toObject();
    foreach (auto osName, jNatives.keys()) {
        natives.insert(osName, jNatives[osName].toString());
    }
}
