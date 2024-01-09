#include <QtCore/QJsonArray>
#include "libraryinfo.h"

Ttyh::Json::LibraryInfo::LibraryInfo(const QJsonObject &jObject)
    : name(jObject["name"].toString(""))
{
    for (const auto& jRule : jObject["rules"].toArray()) {
        rules << Rule(jRule.toObject());
    }

    auto jNatives = jObject["natives"].toObject();
    for (const auto& osName : jNatives.keys()) {
        natives.insert(osName, jNatives[osName].toString());
    }
}
