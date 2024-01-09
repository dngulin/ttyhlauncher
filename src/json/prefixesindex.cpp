#include "prefixesindex.h"

Ttyh::Json::PrefixesIndex::PrefixesIndex(const QJsonObject &jObject)
{
    auto jPrefixes = jObject[keyPrefixes].toObject();

    for (const auto& name : jPrefixes.keys()) {
        prefixes.insert(name, PrefixInfo(jPrefixes[name].toObject()));
    }
}

QJsonObject Ttyh::Json::PrefixesIndex::toJsonObject() const
{
    QJsonObject jPrefixes;

    for (const auto& prefixId : prefixes.keys()) {
        jPrefixes.insert(prefixId, prefixes[prefixId].toJsonObject());
    }

    return QJsonObject { { keyPrefixes, jPrefixes } };
}
