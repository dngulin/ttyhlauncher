#include <QtCore/QJsonArray>
#include "prefixversionsindex.h"

Ttyh::Json::PrefixVersionsIndex::PrefixVersionsIndex(const QJsonObject &jObject)
{
    latest = jObject["latest"]["release"].toString();

    auto jVersions = jObject["versions"].toArray();
    foreach (auto jVersion, jVersions) {
        auto jId = jVersion["id"];
        if (jId.isString())
            versions << jId.toString();
    }
}
