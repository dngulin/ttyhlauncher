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

    jObject.insert(keyUsername, username);
    jObject.insert(keyPassword, password);
    jObject.insert(keySavePassword, savePassword);

    jObject.insert(keyProfile, profile);
    jObject.insert(keyTicket, ticket);

    jObject.insert(keyWindowSizeW, windowSize.width());
    jObject.insert(keyWindowSizeH, windowSize.height());
    jObject.insert(keyWindowMaximized, windowMaximized);
    jObject.insert(keyHideWindowOnRun, hideWindowOnRun);

    return jObject;
}
