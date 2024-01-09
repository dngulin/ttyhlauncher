#include <QtCore/QJsonArray>

#include "filesindex.h"

Ttyh::Profiles::FilesIndex::FilesIndex(const QJsonObject &jObject)
{
    for (const auto& name : jObject[keyInstalled].toArray()) {
        installed << name.toString("");
    }
}

QJsonObject Ttyh::Profiles::FilesIndex::toJsonObject() const
{
    QJsonArray jArray;
    for (const auto& name : installed) {
        jArray << name;
    }

    return QJsonObject { { keyInstalled, jArray } };
}
