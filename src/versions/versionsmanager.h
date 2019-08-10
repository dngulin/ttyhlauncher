#ifndef VERSIONSMANAGER_H
#define VERSIONSMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QHash>
#include <QtCore/QQueue>
#include <QtNetwork/QNetworkAccessManager>

#include "json/prefixesindex.h"
#include "json/checkinfo.h"
#include "logs/logger.h"
#include "logs/namedlogger.h"
#include "storage/fileinfo.h"
#include "utils/downloadfilereply.h"
#include "fullversionid.h"
#include "prefix.h"

namespace Ttyh {
namespace Versions {
class VersionsManager : public QObject
{
    Q_OBJECT
public:
    VersionsManager(const QString &dirName, QString url, QSharedPointer<QNetworkAccessManager> nam,
                    const QSharedPointer<Logs::Logger> &logger);

    void fetchPrefixes();
    void fetchVersionIndexes(const FullVersionId &version);

    QHash<QString, Prefix> getPrefixes() const;
    bool fillVersionFiles(const FullVersionId &version, QList<Storage::FileInfo> &files);

signals:
    void onFetchPrefixesResult(bool result);
    void onFetchVersionIndexesResult(bool result);

private:
    const QString storeUrl;
    QSharedPointer<QNetworkAccessManager> nam;
    Logs::NamedLogger log;

    QString dataPath;
    QString versionsPath;

    QString indexPath;
    Json::PrefixesIndex index;
    QHash<QString, Prefix> prefixes;

    bool fetchingPrefixes;
    QQueue<QString> prefixFetchQueue;

    bool fetchingVersionIndexes;

    void findLocalVersions(const QString &prefixId);
    void fetchNextPrefixOrFinish();
    void setFetchPrefixesResult(bool result);

    void fetchVersionMainIndex(const FullVersionId &version);
    void fetchVersionAssetsIndex(const FullVersionId &version, const QString &assets);
    void fetchVersionDataIndex(const FullVersionId &version);
    void setFetchVersionIndexesResult(bool result);

    QNetworkReply *makeGetRequest(const QString &url);
    Utils::DownloadFileReply *makeGetFileRequest(const QString &url, const QString &path);

    Storage::FileInfo getFileInfo(const QString &location, const FullVersionId &version,
                                  const QString &name, const Json::CheckInfo &checkInfo);

    Storage::FileInfo getFileInfo(const QString &location, const QString &name,
                                  const Json::CheckInfo &checkInfo);
};
}
}

#endif // VERSIONSMANAGER_H
