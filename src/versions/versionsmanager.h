#ifndef VERSIONSMANAGER_H
#define VERSIONSMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QHash>
#include <QtCore/QQueue>
#include <QtNetwork/QNetworkAccessManager>

#include "json/prefixesindex.h"
#include "logs/logger.h"
#include "logs/namedlogger.h"
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

    const QHash<QString, Prefix> getPrefixes() const;

signals:
    void onFetchPrefixesResult(bool result);
    void onFetchVersionIndexesResult(bool result);

private:
    const QString storeUrl;
    QSharedPointer<QNetworkAccessManager> nam;
    Logs::NamedLogger log;

    QString versionsPath;
    QString assetsPath;
    QString librariesPath;

    QString indexPath;
    Json::PrefixesIndex index;
    QHash<QString, Prefix> prefixes;

    bool fetchingPrefixes;
    QQueue<QString> prefixFetchQueue;

    void findLocalVersions(const QString &prefixId);
    void fetchNextPrefixOrFinish();
    void setFetchPrefixesResult(bool result);

    QNetworkReply *makeGetRequest(const QString &url);
};
}
}

#endif // VERSIONSMANAGER_H
