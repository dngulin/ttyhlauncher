#include <QtCore/QStandardPaths>
#include <QtCore/QSettings>
#include "logs/namedlogger.h"
#include "migrations.h"

void Ttyh::Utils::Migrations::restoreLoginSettings(
        const QSharedPointer<Settings::SettingsManager> &settings,
        const QSharedPointer<Logs::Logger> &logger)
{
    Logs::NamedLogger log(logger, "Migrations");
    log.info("Looking for the version 1 settings...");

    auto basePath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    auto configPath = QString("%1/%2/%3").arg(basePath, "ttyhlauncher", "config.ini");

    if (!QFile::exists(configPath)) {
        log.info("Config is not found!");
        return;
    }

    QSettings oldSettings(configPath, QSettings::IniFormat);
    auto userName = oldSettings.value("launcher/login", "Player").toString();
    auto encodedPassword = oldSettings.value("launcher/password", "").toString().toUtf8();
    auto password = QString::fromUtf8(QByteArray::fromBase64(encodedPassword));

    settings->data.username = userName;
    settings->data.password = password;
    settings->data.savePassword = oldSettings.value("launcher/save_password", true).toBool();

    log.info(QString("Login settings for the player '%1' have been restored!").arg(userName));
    settings->save();
}
