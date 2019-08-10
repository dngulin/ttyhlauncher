#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QDir>
#include <QtCore/QSet>
#include <QtCore/QCryptographicHash>

#include "json/dataindex.h"
#include "utils/indexhelper.h"
#include "profilesmanager.h"
#include "filesindex.h"

namespace Ttyh {
namespace Profiles {
ProfilesManager::ProfilesManager(const QString &dirName, const QSharedPointer<Logs::Logger> &logger)
    : dataPath([&dirName]() {
          auto basePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
          return QString("%1/%2").arg(basePath, dirName);
      }()),
      log(logger, "Profiles")
{
    auto profilesPath = QString("%1/%2").arg(dataPath, "profiles");
    auto profilesDir = QDir(profilesPath);

    if (profilesDir.exists()) {
        foreach (auto profileName, profilesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            auto path = QString("%1/%2/%3").arg(profilesPath, profileName, profileIndexName);
            QFile file(path);

            if (!file.exists() || !file.open(QIODevice::ReadOnly))
                continue;

            auto doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject())
                profiles.insert(profileName, ProfileData(doc.object()));
        }
    }

    log.info(QString("Initialized with %1 profile(s)").arg(QString::number(profiles.count())));
}

bool ProfilesManager::isEmpty() const
{
    return profiles.isEmpty();
}

QStringList ProfilesManager::names() const
{
    return profiles.keys();
}

bool ProfilesManager::contains(const QString &profileName) const
{
    return profiles.contains(profileName);
}

ProfileData ProfilesManager::get(const QString &profileName) const
{
    return profiles[profileName];
}

CreateResult ProfilesManager::create(const QString &profileName, const ProfileData &data)
{
    log.info(QString("Create new profile '%1'").arg(profileName));

    if (profiles.contains(profileName)) {
        log.error(QString("Profile with name '%1' already exists in cache!").arg(profileName));
        return CreateResult::AlreadyExists;
    }

    if (!validateProfileName(profileName)) {
        log.error("Invalid profile name!");
        return CreateResult::InvalidName;
    }

    auto profileDirPath = QString("%1/profiles/%2").arg(dataPath, profileName);
    auto profileDir = QDir(profileDirPath);

    if (!profileDir.exists())
        profileDir.mkpath(profileDirPath);

    QFile file(QString("%1/%2.json").arg(profileDirPath, profileIndexName));
    if (!file.open(QIODevice::WriteOnly)) {
        log.error("Failed to save profile!");
        return CreateResult::IOError;
    }

    file.write(QJsonDocument(data.toJsonObject()).toJson());

    profiles.insert(profileName, data);
    return CreateResult::Success;
}

RenameResult ProfilesManager::rename(const QString &oldName, const QString &newName)
{
    log.info(QString("Rename '%1' -> '%2'").arg(oldName, newName));
    if (!profiles.contains(oldName)) {
        log.error(QString("Profile with name '%1' does not exist!").arg(oldName));
        return RenameResult::OldNameDoesNotExist;
    }

    if (profiles.contains(newName)) {
        log.error(QString("Profile with name '%1' already exists!").arg(newName));
        return RenameResult::NewNameAlreadyExists;
    }

    if (!validateProfileName(newName)) {
        log.error("Invalid new profile name!");
        return RenameResult::InvalidName;
    }

    auto dirPath = QString("%1/profiles/%2");
    if (!QDir().rename(dirPath.arg(dataPath, oldName), dirPath.arg(dataPath, newName))) {
        log.error("Failed to rename a directory!");
        return RenameResult::IOError;
    }

    profiles.insert(newName, profiles.take(oldName));
    return RenameResult::Success;
}

bool ProfilesManager::update(const QString &profileName, const ProfileData &data)
{
    log.info(QString("Update profile '%1'").arg(profileName));

    if (!profiles.contains(profileName)) {
        log.error(QString("Profile with name '%1' does not exist!").arg(profileName));
        return false;
    }

    auto path = QString("%1/profiles/%2/%3").arg(dataPath, profileName, profileIndexName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        log.error("Failed to write a profile index");
        return false;
    }

    file.write(QJsonDocument(data.toJsonObject()).toJson());
    profiles[profileName] = data;
    return true;
}

void ProfilesManager::remove(const QString &profileName)
{
    QDir().remove(QString("%1/profiles/%2").arg(dataPath, profileName));
    profiles.remove(profileName);
}

bool ProfilesManager::validateProfileName(const QString &profileName)
{
    if (profileName.trimmed().isEmpty())
        return false;

    QSet<QChar> allowed { ' ', '-', '_', '.', ',', '!', '?' };
    foreach (auto ch, profileName) {
        if (ch.isLetterOrNumber() || allowed.contains(ch))
            continue;

        return false;
    }

    return true;
}

bool ProfilesManager::installFiles(const QString &profileName,
                                   const Versions::FullVersionId &version)
{
    log.info(QString("Updating version files (%1)...").arg(version.toString()));

    auto dataIndexPath = QString("%1/versions/%2/data.json").arg(dataPath, version.toString());
    auto dataIndex = Utils::IndexHelper::load<Json::DataIndex>(dataIndexPath);

    if (!dataIndex.isValid()) {
        log.error("Failed to load the data index!");
        return false;
    }

    auto profilePath = QString("%1/profiles/%2").arg(dataPath, profileName);
    auto filesIndexPath = QString("%1/%2").arg(profilePath, filesIndexName);

    auto filesIndex = Utils::IndexHelper::load<FilesIndex>(filesIndexPath);

    auto actual = QSet<QString>::fromList(dataIndex.files.keys());
    auto obsolete = QSet<QString>::fromList(filesIndex.installed).subtract(actual);

    foreach (auto fileName, obsolete) {
        QFile::remove(QString("%1/%2").arg(profilePath, fileName));
    }

    if (dataIndex.files.isEmpty()) {
        QFile::remove(filesIndexPath);
        return true;
    }

    auto mutableFiles = QSet<QString>::fromList(dataIndex.mutableFiles);

    auto srcPattern = QString("%1/versions/%2/files/%3");
    auto dstPattern = QString("%1/profiles/%2/%3");

    foreach (auto fileName, dataIndex.files.keys()) {
        auto checkInfo = dataIndex.files[fileName];

        auto src = srcPattern.arg(dataPath, version.toString(), fileName);
        auto dst = dstPattern.arg(dataPath, profileName, fileName);

        if (checkFile(dst, checkInfo.size, checkInfo.hash, mutableFiles.contains(fileName)))
            continue;

        auto dir = QFileInfo(dst).absoluteDir();
        dir.mkpath(dir.absolutePath());

        if (QFile::exists(dst))
            QFile::remove(dst);

        QFile::copy(src, dst);
    }

    filesIndex.installed = dataIndex.files.keys();
    return Utils::IndexHelper::save(filesIndex, filesIndexPath);
}

bool ProfilesManager::checkFile(const QString &path, int size, const QString &hash, bool editable)
{
    QFile file(path);

    if (!file.exists())
        return false;

    if (editable)
        return true;

    if (file.size() != size)
        return false;

    QCryptographicHash sha1(QCryptographicHash::Sha1);

    file.open(QIODevice::ReadOnly);
    sha1.addData(&file);

    return hash == sha1.result().toHex();
}

}
}
