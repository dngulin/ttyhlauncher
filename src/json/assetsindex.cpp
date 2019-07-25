#include "assetsindex.h"

Ttyh::Json::AssetsIndex::AssetsIndex(const QJsonObject &jObject)
{
    auto jObjects = jObject[keyObjects].toObject();

    foreach(auto name, jObjects.keys()) {
        objects.insert(name, CheckInfo(jObjects[name].toObject()));
    }
}
