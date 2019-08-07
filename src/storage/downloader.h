#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QPointer>
#include <QtNetwork/QNetworkAccessManager>

#include "logs/logger.h"
#include "logs/namedlogger.h"
#include "fileinfo.h"
#include "downloadfilereply.h"

namespace Ttyh {
namespace Storage {
class Downloader : public QObject
{
    Q_OBJECT
public:
    Downloader(QString storageUrl, const QString &dirName,
               QSharedPointer<QNetworkAccessManager> nam,
               const QSharedPointer<Logs::Logger> &logger);

    void start(const QList<FileInfo> &files);
    void cancel();

signals:
    void rangeChanged(int min, int max);
    void progressChanged(int currentId, const QString &currentName);
    void finished(bool cancelled, bool result);

private:
    const QString storeUrl;
    const QString dataPath;
    const QString versionsPath;

    int prefixLength;
    bool downloading;

    QString currentFile;
    int currentSize;
    int totalSize;

    QSharedPointer<QNetworkAccessManager> nam;
    QPointer<DownloadFileReply> currentReply;

    QQueue<Storage::FileInfo> downloadQueue;

    Logs::NamedLogger log;

    void downloadNextFileOrFinish();
    void finish(bool cancelled, bool result);
};
}
}

#endif // DOWNLOADER_H
