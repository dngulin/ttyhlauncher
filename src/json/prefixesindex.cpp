#include "prefixesindex.h"

Ttyh::Json::PrefixesIndex::PrefixesIndex(const QJsonObject &jObject)
{
    auto jPrefixes = jObject[keyPrefixes].toObject();

    foreach (auto name, jPrefixes.keys()) {
        prefixes.insert(name, PrefixInfo(jPrefixes[name].toObject()));
    }
}

QJsonObject Ttyh::Json::PrefixesIndex::toJsonObject() const
{
    QJsonObject jPrefixes;

    foreach (auto prefixId, prefixes.keys()) {
        jPrefixes.insert(prefixId, prefixes[prefixId].toJsonObject());
    }

    return QJsonObject { { keyPrefixes, jPrefixes } };
}
