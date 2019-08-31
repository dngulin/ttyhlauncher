#include "fullversionid.h"
#include "prefix.h"

Ttyh::Versions::FullVersionId::FullVersionId(QString prefixId, QString versionId)
    : prefix(std::move(prefixId)), id(std::move(versionId))
{
}

Ttyh::Versions::FullVersionId::FullVersionId(const QJsonObject &jObject)
    : prefix(jObject[keyPrefix].toString("default")),
      id(jObject[keyVersion].toString(Prefix::latestVersionAlias))
{
}

QJsonObject Ttyh::Versions::FullVersionId::toJsonObject() const
{
    return QJsonObject { { keyPrefix, prefix }, { keyVersion, id } };
}

QString Ttyh::Versions::FullVersionId::toString() const
{
    return QString("%1/%2").arg(prefix, id);
}
