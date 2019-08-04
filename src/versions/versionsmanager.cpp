#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QDir>
#include <QtNetwork/QNetworkReply>

#include <QtCore/QTimer>
#include <json/versionindex.h>
#include <json/prefixversionsindex.h>
#include "utils/network.h"
#include "versionsmanager.h"

namespace Ttyh {
namespace Versions {
using namespace Json;
using namespace Logs;

VersionsManager::VersionsManager(const QString &dirName, QString url,
                                 QSharedPointer<QNetworkAccessManager> nam,
                                 const QSharedPointer<Logger> &logger)
    : storeUrl(std::move(url)),
      nam(std::move(nam)),
      log(logger, "Versions"),
      fetchingPrefixes(false),
      fetchingVersionIndexes(false)
{
    auto pattern = QString("%1/%2");
    auto basePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

    dataPath = pattern.arg(basePath, dirName);
    versionsPath = pattern.arg(dataPath, "versions");

    QDir().mkpath(versionsPath);

    indexPath = QString("%1/%2").arg(versionsPath, "prefixes.json");
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

        if (!file.open(QIODevice::ReadOnly))
            continue;

        auto versionIndex = VersionIndex(QJsonDocument::fromJson(file.readAll()).object());
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

        auto remoteIndex = PrefixesIndex(QJsonDocument::fromJson(reply->readAll()).object());

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

        auto knownVersions = QSet<QString>();
        foreach (auto versionId, prefix.versions) {
            knownVersions << versionId;
        }
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

    auto locationPattern = QString("%1/assets/indexes/%2.json");
    auto reply = makeGetRequest(locationPattern.arg(storeUrl, assets));

    connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QString msg = "Failed to get the assets index '%1' (%2): %3";
            log.error(msg.arg(assets, version.toString(), reply->errorString()));
            setFetchVersionIndexesResult(false);
            return;
        }

        QDir().mkpath(QString("%1/assets/indexes").arg(dataPath));

        QFile file(locationPattern.arg(dataPath, assets));
        if (!file.open(QIODevice::WriteOnly)) {
            QString msg = "Failed to save the assets index '%1' (%2)";
            log.error(msg.arg(assets, version.toString()));
            setFetchVersionIndexesResult(false);
            return;
        }

        file.write(reply->readAll());
        fetchVersionDataIndex(version);
    });
}

void VersionsManager::fetchVersionDataIndex(const FullVersionId &version)
{
    auto locationPattern = QString("%1/%2/%3/data.json");
    auto reply = makeGetRequest(locationPattern.arg(storeUrl, version.prefix, version.id));

    connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QString msg = "Failed to get the data index '%1': %2";
            log.error(msg.arg(version.toString(), reply->errorString()));
            setFetchVersionIndexesResult(false);
            return;
        }

        QDir().mkpath(QString("%1/%2/%3").arg(versionsPath, version.prefix, version.id));

        QFile file(locationPattern.arg(versionsPath, version.prefix, version.id));
        if (!file.open(QIODevice::WriteOnly)) {
            log.error(QString("Failed to save the data index '%1'").arg(version.toString()));
            setFetchVersionIndexesResult(false);
            return;
        }

        file.write(reply->readAll());
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

const QHash<QString, Prefix> VersionsManager::getPrefixes() const
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

}
}
