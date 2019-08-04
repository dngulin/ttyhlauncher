#include <QtCore/QStringList>
#include "fullversionid.h"

Ttyh::Versions::FullVersionId::FullVersionId(QString prefixId, QString versionId)
    : prefixId(std::move(prefixId)), versionId(std::move(versionId))
{
}

Ttyh::Versions::FullVersionId::FullVersionId(const QJsonObject &jObject)
{
    prefixId = jObject[keyPrefix].toString();
    versionId = jObject[keyVersion].toString();
}

QJsonObject Ttyh::Versions::FullVersionId::toJsonObject() const
{
    return QJsonObject { { keyPrefix, prefixId }, { keyVersion, versionId } };
}
