#include <QtCore/QJsonDocument>
#include <QtCore/QDir>
#include "settingsmanager.h"

using namespace Ttyh::Logs;

Ttyh::Settings::SettingsManager::SettingsManager(const QString &workDir,
                                                 const QSharedPointer<Logger> &logger)
    : cfgFilePath(QString("%1/%2").arg(workDir, "settings.dat")),
      freshRun(!QFile::exists(cfgFilePath)),
      log(logger, "Settings")
{
    auto dir = QFileInfo(cfgFilePath).absoluteDir();
    dir.mkpath(dir.absolutePath());

    QFile file(cfgFilePath);

    if (file.open(QIODevice::ReadOnly)) {
        data = SettingsData(QJsonDocument::fromJson(qUncompress(file.readAll())).object());
    } else {
        log.info("Default settings have been created");
    }

    log.info("Initialized!");
}

bool Ttyh::Settings::SettingsManager::isFreshRun()
{
    return freshRun;
}

void Ttyh::Settings::SettingsManager::save()
{
    QFile file(cfgFilePath);

    if (file.open(QIODevice::WriteOnly)) {
        file.write(qCompress(QJsonDocument(data.toJsonObject()).toJson()));
        log.info("Saved!");
    } else {
        log.error("Failed to save settings file!");
    }
}

Ttyh::Settings::SettingsManager::~SettingsManager()
{
    save();
}
