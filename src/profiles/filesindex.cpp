#include <QtCore/QJsonArray>

#include "filesindex.h"

Ttyh::Profiles::FilesIndex::FilesIndex(const QJsonObject &jObject)
{
    foreach (auto name, jObject[keyInstalled].toArray()) {
        installed << name.toString("");
    }
}

QJsonObject Ttyh::Profiles::FilesIndex::toJsonObject() const
{
    QJsonArray jArray;
    foreach (auto name, installed) {
        jArray << name;
    }

    return QJsonObject { { keyInstalled, jArray } };
}
