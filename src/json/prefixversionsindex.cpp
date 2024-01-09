#include <QtCore/QJsonArray>
#include "prefixversionsindex.h"

Ttyh::Json::PrefixVersionsIndex::PrefixVersionsIndex(const QJsonObject &jObject)
    : latest(jObject["latest"].toObject()["release"].toString(""))
{
    auto jVersions = jObject["versions"].toArray();
    for (const auto& jVersion : jVersions) {
        auto jVersionObj = jVersion.toObject();
        auto version = jVersionObj["id"].toString();
        if (!version.isEmpty())
            versions << version;
    }
}
