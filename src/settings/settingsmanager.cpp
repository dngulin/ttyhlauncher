#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QDir>
#include "settingsmanager.h"

using namespace Ttyh::Logs;

Ttyh::Settings::SettingsManager::SettingsManager(const QString &dirName,
                                                 const QSharedPointer<Logger> &logger):
                                                 log(logger, "Settings")
{
    auto genericCfgDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QString sub("%1/%2");

    auto cfgDir = sub.arg(genericCfgDir, dirName);
    QDir().mkpath(cfgDir);

    cfgFilePath = sub.arg(cfgDir, "settings.json");
    QFile file(cfgFilePath);

    if (file.open(QIODevice::ReadOnly)) {
        data = SettingsData(QJsonDocument::fromJson(file.readAll()).object());
    } else {
        log.info("Default settings have been created");
    }

    log.info("Initialized!");
}

void Ttyh::Settings::SettingsManager::save() {
    QFile file(cfgFilePath);

    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(data.toJsonObject()).toJson());
        log.info("Saved!");
    } else {
        log.error("Failed to save settings file!");
    }
}

Ttyh::Settings::SettingsManager::~SettingsManager() {
    save();
}
