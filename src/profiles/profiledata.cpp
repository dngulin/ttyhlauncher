#include "profiledata.h"

Ttyh::Profiles::ProfileData::ProfileData(const QJsonObject &jObject)
    : version(jObject[keyVersion].toObject()),

      useCustomJavaPath(jObject[keyUseCustomJavaPath].toBool(false)),
      customJavaPath(jObject[keyCustomJavaPath].toString("")),

      useCustomJavaArgs(jObject[keyUseCustomJavaArgs].toBool(false)),
      customJavaArgs(jObject[keyCustomJavaArgs].toString(""))
{
}

QJsonObject Ttyh::Profiles::ProfileData::toJsonObject() const
{
    QJsonObject jObject;

    jObject[keyVersion] = version.toJsonObject();

    jObject[keyUseCustomJavaPath] = useCustomJavaPath;
    jObject[keyCustomJavaPath] = customJavaPath;

    jObject[keyUseCustomJavaArgs] = useCustomJavaArgs;
    jObject[keyCustomJavaArgs] = customJavaArgs;

    return jObject;
}
