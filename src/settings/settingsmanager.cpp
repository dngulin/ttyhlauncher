#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QDir>
#include "settingsmanager.h"

using namespace Ttyh::Logs;

Ttyh::Settings::SettingsManager::SettingsManager(const QString &dirName,
                                                 const QSharedPointer<Logger> &logger)
    : cfgFilePath([&dirName]() {
          auto basePath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
          return QString("%1/%2/%3").arg(basePath, dirName, "settings.dat");
      }()),
      freshRun(!QFile::exists(cfgFilePath)),
      log(logger, "Settings")
{
    auto dir = QFileInfo(cfgFilePath).absoluteDir();
    dir.mkpath(dir.absolutePath());

    QFile file(cfgFilePath);

    if (file.open(QIODevice::ReadOnly)) {
        auto bytes = file.readAll();
        xorBytes(bytes);
        data = SettingsData(QJsonDocument::fromJson(qUncompress(bytes)).object());
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
        auto bytes = qCompress(QJsonDocument(data.toJsonObject()).toJson());
        xorBytes(bytes);
        file.write(bytes);
        log.info("Saved!");
    } else {
        log.error("Failed to save settings file!");
    }
}

Ttyh::Settings::SettingsManager::~SettingsManager()
{
    save();
}

void Ttyh::Settings::SettingsManager::xorBytes(QByteArray &bytes)
{
    auto key = cfgFilePath.toUtf8().toBase64();
    for (int i = 0; i < bytes.length(); i++) {
        bytes[i] = bytes[i] ^ key[i % key.length()];
    }
}
