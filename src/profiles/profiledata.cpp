#include "profiledata.h"

Ttyh::Profiles::ProfileData::ProfileData(const QJsonObject &jObject)
    : version(jObject[keyVersion].toObject()),

      useCustomJavaPath(jObject[keyUseCustomJavaPath].toBool(false)),
      customJavaPath(jObject[keyCustomJavaPath].toString("")),

      useCustomJavaArgs(jObject[keyUseCustomJavaArgs].toBool(false)),
      customJavaArgs(jObject[keyCustomJavaArgs].toString("")),

      setWindowSizeOnRun(jObject[keySetWindowSize].toBool(false)),
      windowSizeMode(stringToSizeMode(jObject[keyWindowSizeMode].toString(modeFullScreen))),
      windowSize(jObject[keyWindowWidth].toInt(800), jObject[keyWindowHeight].toInt(600))
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

    jObject.insert(keySetWindowSize, setWindowSizeOnRun);
    jObject.insert(keyWindowSizeMode, sizeModeToString(windowSizeMode));

    jObject.insert(keyWindowWidth, windowSize.width());
    jObject.insert(keyWindowHeight, windowSize.height());

    return jObject;
}

Ttyh::Profiles::WindowSizeMode
Ttyh::Profiles::ProfileData::stringToSizeMode(const QString &stringMode)
{
    if (stringMode == modeLauncherLike)
        return WindowSizeMode::LauncherLike;

    if (stringMode == modeSpecifiedSize)
        return WindowSizeMode::Specified;

    return WindowSizeMode::FullScreen;
}

QString Ttyh::Profiles::ProfileData::sizeModeToString(WindowSizeMode mode)
{
    switch (mode) {
    case WindowSizeMode::LauncherLike:
        return modeLauncherLike;

    case WindowSizeMode::Specified:
        return modeSpecifiedSize;

    default:
        return modeFullScreen;
    }
}
