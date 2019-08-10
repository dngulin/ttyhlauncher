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

    jObject.insert(keyVersion, version.toJsonObject());

    jObject.insert(keyUseCustomJavaPath, useCustomJavaPath);
    jObject.insert(keyCustomJavaPath, customJavaPath);

    jObject.insert(keyUseCustomJavaArgs, useCustomJavaArgs);
    jObject.insert(keyCustomJavaArgs, customJavaArgs);

    return jObject;
}
