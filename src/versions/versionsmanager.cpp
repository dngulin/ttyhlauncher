#include <QtCore/QJsonDocument>
#include <QtCore/QDir>
#include <QtNetwork/QNetworkReply>

#include "storage/fileinfo.h"
#include "json/assetsindex.h"
#include "json/versionindex.h"
#include "json/prefixversionsindex.h"
#include "json/dataindex.h"
#include "utils/network.h"
#include "utils/platform.h"
#include "utils/indexhelper.h"
#include "versionsmanager.h"

namespace Ttyh {
namespace Versions {
using namespace Json;
using namespace Logs;
using namespace Storage;
using namespace Utils;

VersionsManager::VersionsManager(QString workDir, QString url,
                                 QSharedPointer<QNetworkAccessManager> nam,
                                 const QSharedPointer<Logger> &logger)
    : dataPath(std::move(workDir)),
      versionsPath(QString("%1/%2").arg(dataPath, "versions")),
      indexPath(QString("%1/%2").arg(versionsPath, "prefixes.json")),
      storeUrl(std::move(url)),
      nam(std::move(nam)),
      log(logger, "Versions"),
      fetchingPrefixes(false),
      fetchingVersionIndexes(false)
{
    QDir().mkpath(versionsPath);
    QFile indexFile(indexPath);

    if (indexFile.open(QIODevice::ReadOnly)) {
        index = PrefixesIndex(QJsonDocument::fromJson(indexFile.readAll()).object());
    } else {
        log.info("Default version index have been created");
    }

    foreach (auto id, index.prefixes.keys()) {
        if (id.isEmpty())
            continue;

        prefixes.insert(id, Prefix(id, index.prefixes[id].about));
        findLocalVersions(id);
    }

    log.info(QString("Initialized with %1 prefix(es)").arg(index.prefixes.count()));
}

void VersionsManager::findLocalVersions(const QString &prefixId)
{
    Prefix &prefix = prefixes[prefixId];
    auto prefixPath = QString("%1/%2").arg(versionsPath, prefixId);
    auto versions = QDir(prefixPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (auto versionId, versions) {
        auto versionIndexPath = QString("%1/%2/%2.json").arg(prefixPath, versionId);
        QFile file(versionIndexPath);

        if (!file.exists() || !file.open(QIODevice::ReadOnly))
            continue;

        auto doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject())
            continue;

        auto versionIndex = VersionIndex(doc.object());
        if (versionIndex.id != versionId) {
            auto msg = QString("A version index '%1' contains the wrong version id '%2'");
            log.warning(msg.arg(versionIndexPath, versionIndex.id));
            continue;
        }

        prefix.versions << versionId;
        log.info(QString("Local version is found: '%1/%2'").arg(prefixId, versionId));
    }

    std::sort(prefix.versions.begin(), prefix.versions.end(), std::greater<QString>());

    if (prefix.versions.count() > 1)
        prefix.latestVersionId = prefix.versions[1];
}

void VersionsManager::fetchPrefixes()
{
    if (fetchingPrefixes) {
        log.warning("Failed to start a prefixes fetching! Already in progress!");
        return;
    }

    log.info("Fetching actual prefixes...");
    fetchingPrefixes = true;

    auto reply = makeGetRequest(QString("%1/prefixes.json").arg(storeUrl));

    connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            log.error("Failed to get the prefixes index: " + reply->errorString());
            setFetchPrefixesResult(false);
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            log.error("Failed to parse the prefixes index!");
            setFetchPrefixesResult(false);
            return;
        }

        auto remoteIndex = PrefixesIndex(doc.object());
        if (remoteIndex.prefixes.isEmpty()) {
            log.error("Invalid prefixes index!");
            setFetchPrefixesResult(false);
            return;
        }

        foreach (auto id, remoteIndex.prefixes.keys()) {
            index.prefixes[id] = remoteIndex.prefixes[id];
            prefixFetchQueue << id;

            if (!prefixes.contains(id))
                prefixes.insert(id, Prefix(id, remoteIndex.prefixes[id].about));
        }

        QFile indexFile(indexPath);
        if (indexFile.open(QIODevice::WriteOnly)) {
            indexFile.write(QJsonDocument(index.toJsonObject()).toJson());
        } else {
            log.error("Failed to save the prefixes index file!");
            setFetchPrefixesResult(false);
            return;
        }

        fetchNextPrefixOrFinish();
    });
}

void VersionsManager::fetchNextPrefixOrFinish()
{
    if (prefixFetchQueue.isEmpty()) {
        setFetchPrefixesResult(true);
        return;
    }

    auto prefixId = prefixFetchQueue.dequeue();
    auto reply = makeGetRequest(QString("%1/%2/versions/versions.json").arg(storeUrl, prefixId));

    connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QString msg = "Failed to get the versions index for the prefix '%1': %2";
            log.error(msg.arg(prefixId, reply->errorString()));
            setFetchPrefixesResult(false);
            return;
        }

        auto indexJson = QJsonDocument::fromJson(reply->readAll()).object();
        auto versionsIndex = Json::PrefixVersionsIndex(indexJson);

        Prefix &prefix = prefixes[prefixId];
        prefix.latestVersionId = versionsIndex.latest;

        auto knownVersions = QSet<QString>::fromList(prefix.versions);
        foreach (auto versionId, versionsIndex.versions) {
            if (!knownVersions.contains(versionId))
                prefix.versions << versionId;
        }

        std::sort(prefix.versions.begin(), prefix.versions.end(), std::greater<QString>());

        fetchNextPrefixOrFinish();
    });
}

void VersionsManager::setFetchPrefixesResult(bool result)
{
    if (result) {
        log.info("All prefixes are successfully fetched!");
    }

    prefixFetchQueue.clear();
    fetchingPrefixes = false;

    emit onFetchPrefixesResult(result);
}

void VersionsManager::fetchVersionIndexes(const FullVersionId &version)
{
    if (fetchingVersionIndexes) {
        log.warning("Failed to start a version indexes fetching! Already in progress!");
        return;
    }

    log.info(QString("Fetching actual indexes for the '%1'...").arg(version.toString()));
    fetchingVersionIndexes = true;

    fetchVersionMainIndex(version);
}

void VersionsManager::fetchVersionMainIndex(const FullVersionId &version)
{
    auto locationPattern = QString("%1/%2/%3/%3.json");
    auto reply = makeGetRequest(locationPattern.arg(storeUrl, version.prefix, version.id));

    connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QString msg = "Failed to get the version index '%1': %2";
            log.error(msg.arg(version.toString(), reply->errorString()));
            setFetchVersionIndexesResult(false);
            return;
        }

        QDir().mkpath(QString("%1/%2/%3").arg(versionsPath, version.prefix, version.id));

        QFile file(locationPattern.arg(versionsPath, version.prefix, version.id));
        if (!file.open(QIODevice::WriteOnly)) {
            log.error(QString("Failed to save the version index '%1'").arg(version.toString()));
            setFetchVersionIndexesResult(false);
            return;
        }

        auto data = reply->readAll();
        file.write(data);

        auto versionIndex = VersionIndex(QJsonDocument::fromJson(data).object());
        fetchVersionAssetsIndex(version, versionIndex.assetsIndex);
    });
}

void VersionsManager::fetchVersionAssetsIndex(const FullVersionId &version, const QString &assets)
{
    if (assets.isEmpty()) {
        auto msg = QString("Failed to resolve assets index path for the version '%1'");
        log.error(msg.arg(version.toString()));
        setFetchVersionIndexesResult(false);
        return;
    }

    auto location = QString("%1/assets/indexes/%2.json");
    auto reply = makeGetFileRequest(location.arg(storeUrl, assets), location.arg(dataPath, assets));

    connect(reply, &DownloadFileReply::finished, [=](bool cancelled, bool result) {
        if (!result || cancelled) {
            QString msg = "Failed to save the assets index '%1' (%2): %3";
            log.error(msg.arg(assets, version.toString(), reply->errorString()));
            setFetchVersionIndexesResult(false);
            return;
        }

        fetchVersionDataIndex(version);
    });
}

void VersionsManager::fetchVersionDataIndex(const FullVersionId &version)
{
    auto location = QString("%1/%2/%3/data.json");
    auto url = location.arg(storeUrl, version.prefix, version.id);
    auto path = location.arg(versionsPath, version.prefix, version.id);

    auto reply = makeGetFileRequest(url, path);
    connect(reply, &DownloadFileReply::finished, [=](bool cancelled, bool result) {
        if (!result || cancelled) {
            QString msg = "Failed to save the data index '%1': %2";
            log.error(msg.arg(version.toString(), reply->errorString()));
            setFetchVersionIndexesResult(false);
            return;
        }

        setFetchVersionIndexesResult(true);
    });
}

void VersionsManager::setFetchVersionIndexesResult(bool result)
{
    if (result) {
        log.info("All indexes are successfully fetched!");
    }

    fetchingVersionIndexes = false;
    emit onFetchVersionIndexesResult(result);
}

QHash<QString, Prefix> VersionsManager::getPrefixes() const
{
    return prefixes;
}

QNetworkReply *VersionsManager::makeGetRequest(const QString &url)
{
    log.info(QString("Requesting '%1'...").arg(url));
    auto reply = nam->get(QNetworkRequest(url));
    Utils::Network::createTimeoutTimer(reply);

    return reply;
}

DownloadFileReply *VersionsManager::makeGetFileRequest(const QString &url, const QString &path)
{
    return new DownloadFileReply(makeGetRequest(url), path);
}

bool VersionsManager::fillVersionFiles(const FullVersionId &version, QList<FileInfo> &files)
{
    log.info(QString("Collecting files for the version '%1'...").arg(version.toString()));

    auto dIndexPath = QString("%1/%2/data.json").arg(versionsPath, version.toString());
    auto dataIndex = IndexHelper::load<Json::DataIndex>(dIndexPath);
    if (!dataIndex.isValid()) {
        log.error("Failed to load data index!");
        return false;
    }

    files << getFileInfo("%1/%2/%3.jar", version, version.id, dataIndex.main);

    foreach (auto file, dataIndex.files.keys()) {
        files << getFileInfo("%1/%2/files/%3", version, file, dataIndex.files[file]);
    }

    auto vIndexPath = QString("%1/%2/%3.json").arg(versionsPath, version.toString(), version.id);
    auto versionIndex = IndexHelper::load<Json::VersionIndex>(vIndexPath);
    if (!versionIndex.isValid()) {
        log.error("Failed to load version index!");
        return false;
    }

    foreach (auto libInfo, versionIndex.libraries) {
        if (!Utils::Platform::isLibraryAllowed(libInfo))
            continue;

        auto libPath = Utils::Platform::getLibraryPath(libInfo);
        if (!dataIndex.libs.contains(libPath)) {
            log.warning(QString("Library '%1' is missing in the data index").arg(libPath));
            continue;
        }

        files << getFileInfo("%1/libraries/%2", libPath, dataIndex.libs[libPath]);
    }

    auto aIndexPath = QString("%1/assets/indexes/%2.json").arg(dataPath, versionIndex.assetsIndex);
    auto assetsIndex = IndexHelper::load<Json::AssetsIndex>(aIndexPath);
    if (!assetsIndex.isValid()) {
        log.error("Failed to load version index!");
        return false;
    }

    foreach (auto asset, assetsIndex.objects) {
        auto name = QString("%1/%2").arg(asset.hash.mid(0, 2), asset.hash);
        files << getFileInfo("%1/assets/objects/%2", name, asset);
    }

    log.info(QString("Need to check %1 files").arg(QString::number(files.count())));
    return true;
}

FileInfo VersionsManager::getFileInfo(const QString &location, const FullVersionId &version,
                                      const QString &name, const CheckInfo &checkInfo)
{
    auto url = location.arg(storeUrl, version.toString(), name);
    auto path = location.arg(versionsPath, version.toString(), name);
    return FileInfo(url, path, checkInfo.hash, checkInfo.size);
}

FileInfo VersionsManager::getFileInfo(const QString &location, const QString &name,
                                      const CheckInfo &checkInfo)
{
    auto url = location.arg(storeUrl, name);
    auto path = location.arg(dataPath, name);
    return FileInfo(url, path, checkInfo.hash, checkInfo.size);
}

FullVersionId VersionsManager::resolve(const FullVersionId &version) const
{
    auto prefix = version.prefix;

    if (version.id != Prefix::latestVersionAlias || !prefixes.contains(prefix))
        return version;

    return FullVersionId(prefix, prefixes[prefix].latestVersionId);
}

}
}
