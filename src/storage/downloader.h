#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QPointer>
#include <QtNetwork/QNetworkAccessManager>

#include "logs/logger.h"
#include "logs/namedlogger.h"
#include "fileinfo.h"
#include "utils/downloadfilereply.h"

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

    QString currentFileName;
    quint64 currentSize;
    quint64 totalSize;

    QSharedPointer<QNetworkAccessManager> nam;
    QPointer<Utils::DownloadFileReply> currentReply;

    QQueue<Storage::FileInfo> downloadQueue;

    Logs::NamedLogger log;

    void downloadNextFileOrFinish();
    void finish(bool cancelled, bool result);

    int getRangedProgress() const;
    static const int progressRange = 1000;
};
}
}

#endif // DOWNLOADER_H
