#include <QtCore/QJsonArray>
#include "dataindex.h"

Ttyh::Json::DataIndex::DataIndex(const QJsonObject &jObject) : main(jObject["main"].toObject())
{
    auto jLibs = jObject["libs"].toObject();
    foreach (auto path, jLibs.keys()) {
        libs.insert(path, CheckInfo(jLibs[path].toObject()));
    }

    auto jFiles = jObject["files"].toObject()["index"].toObject();
    foreach (auto path, jFiles.keys()) {
        files.insert(path, CheckInfo(jFiles[path].toObject()));
    }

    auto jMutableFiles = jObject["files"].toObject()["mutables"].toArray();
    foreach (auto path, jMutableFiles) {
        mutableFiles << path.toString();
    }
}

bool Ttyh::Json::DataIndex::isValid() const
{
    return main.isValid() && !libs.isEmpty();
}
