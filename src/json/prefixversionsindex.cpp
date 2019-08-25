#include <QtCore/QJsonArray>
#include "prefixversionsindex.h"

Ttyh::Json::PrefixVersionsIndex::PrefixVersionsIndex(const QJsonObject &jObject)
    : latest(jObject["latest"].toObject()["release"].toString(""))
{
    auto jVersions = jObject["versions"].toArray();
    foreach (auto jVersion, jVersions) {
        auto jId = jVersion.toObject()["id"];
        if (jId.isString())
            versions << jId.toString();
    }
}
