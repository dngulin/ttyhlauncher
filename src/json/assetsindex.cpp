#include "assetsindex.h"

Ttyh::Json::AssetsIndex::AssetsIndex(const QJsonObject &jObject)
{
    auto jObjects = jObject["objects"].toObject();

    for (const auto& name : jObjects.keys()) {
        objects.insert(name, CheckInfo(jObjects[name].toObject()));
    }
}

bool Ttyh::Json::AssetsIndex::isValid() const
{
    return !objects.isEmpty();
}
