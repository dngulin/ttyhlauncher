#include <QtCore/QUuid>
#include "settingsdata.h"

Ttyh::Settings::SettingsData::SettingsData(const QJsonObject &jObject)
{
    username = jObject[keyUsername].toString("Player");
    password = jObject[keyPassword].toString("");
    savePassword = jObject[keySavePassword].toBool(true);

    profile = jObject[keyProfile].toString("");
    revision = jObject[keyRevision].toString(QUuid::createUuid().toString(QUuid::WithoutBraces));

    windowSize = QSize(jObject[keyWindowSizeW].toInt(800), jObject[keyWindowSizeH].toInt(600));
    windowMaximized = jObject[keyWindowMaximized].toBool(false);
    hideWindowOnRun = jObject[keyHideWindowOnRun].toBool(true);
}

QJsonObject Ttyh::Settings::SettingsData::toJsonObject() const
{
    auto jObject = QJsonObject();

    jObject[keyUsername] = username;
    jObject[keyPassword] = password;
    jObject[keySavePassword] = savePassword;

    jObject[keyProfile] = profile;
    jObject[keyRevision] = revision;

    jObject[keyWindowSizeW] = windowSize.width();
    jObject[keyWindowSizeH] = windowSize.height();
    jObject[keyWindowMaximized] = windowMaximized;
    jObject[keyHideWindowOnRun] = keyHideWindowOnRun;

    return jObject;
}
