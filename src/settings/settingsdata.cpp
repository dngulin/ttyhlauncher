#include <QtCore/QUuid>
#include "settingsdata.h"

Ttyh::Settings::SettingsData::SettingsData(const QJsonObject &jObject)
    : username(jObject[keyUsername].toString("Player")),
      password(jObject[keyPassword].toString("")),
      savePassword(jObject[keySavePassword].toBool(true)),
      profile(jObject[keyProfile].toString("")),
      ticket(jObject[keyTicket].toString(QUuid::createUuid().toString(QUuid::WithoutBraces))),
      windowSize(jObject[keyWindowSizeW].toInt(800), jObject[keyWindowSizeH].toInt(600)),
      windowMaximized(jObject[keyWindowMaximized].toBool(false)),
      hideWindowOnRun(jObject[keyHideWindowOnRun].toBool(true))
{
}

QJsonObject Ttyh::Settings::SettingsData::toJsonObject() const
{
    QJsonObject jObject;

    jObject[keyUsername] = username;
    jObject[keyPassword] = password;
    jObject[keySavePassword] = savePassword;

    jObject[keyProfile] = profile;
    jObject[keyTicket] = ticket;

    jObject[keyWindowSizeW] = windowSize.width();
    jObject[keyWindowSizeH] = windowSize.height();
    jObject[keyWindowMaximized] = windowMaximized;
    jObject[keyHideWindowOnRun] = hideWindowOnRun;

    return jObject;
}
