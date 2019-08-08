#include <QtCore/QStandardPaths>
#include "downloader.h"

using namespace Ttyh::Utils;

Ttyh::Storage::Downloader::Downloader(QString storageUrl, const QString &dirName,
                                      QSharedPointer<QNetworkAccessManager> nam,
                                      const QSharedPointer<Ttyh::Logs::Logger> &logger)
    : storeUrl(std::move(storageUrl)),
      dataPath([&dirName]() {
          auto basePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
          return QString("%1/%2").arg(basePath, dirName);
      }()),
      versionsPath(QString("%1/%2").arg(dataPath, "versions")),
      prefixLength(dataPath.length() + 1),
      downloading(false),
      currentFile(""),
      currentSize(0),
      totalSize(0),
      nam(std::move(nam)),
      currentReply(),
      downloadQueue(),
      log(logger, "Downloader")
{
}

void Ttyh::Storage::Downloader::cancel()
{
    if (!downloading) {
        log.warning("Cancellation is ignored. Task is not running!");
        return;
    }

    if (currentReply)
        currentReply->cancel();
}

void Ttyh::Storage::Downloader::start(const QList<Ttyh::Storage::FileInfo> &files)
{
    if (downloading) {
        log.error("Failed to start a downloading. Already in progress!");
        return;
    }

    log.info("Start downloading files...");
    downloading = true;
    currentFile = "";
    currentSize = 0;
    totalSize = 0;

    foreach (auto fileInfo, files) {
        totalSize += fileInfo.size;
        downloadQueue << fileInfo;
    }

    emit rangeChanged(0, totalSize);
    emit progressChanged(currentSize, currentFile);

    downloadNextFileOrFinish();
}

void Ttyh::Storage::Downloader::downloadNextFileOrFinish()
{
    if (downloadQueue.isEmpty()) {
        finish(false, true);
        return;
    }

    auto target = downloadQueue.dequeue();

    log.info(QString("Downloading '%1'...").arg(target.url));
    currentFile = target.path.mid(prefixLength);

    auto reply = new DownloadFileReply(nam->get(QNetworkRequest(target.url)), target.path);
    currentReply = QPointer<DownloadFileReply>(reply);

    connect(reply, &DownloadFileReply::bytesWritten, [=](int written) {
        if (!downloading)
            return;

        currentSize += written;
        emit progressChanged(currentSize, currentFile);
    });

    connect(reply, &DownloadFileReply::finished, [=](bool cancelled, bool result) {
        if (cancelled || !result) {

            if (!result) {
                auto msg = QString("Failed to download '%1': %2");
                log.error(msg.arg(currentFile, currentReply->errorString()));
            }

            finish(cancelled, result);
            return;
        }

        downloadNextFileOrFinish();
    });
}

void Ttyh::Storage::Downloader::finish(bool cancelled, bool result)
{
    downloading = false;

    currentFile = "";
    currentSize = 0;
    totalSize = 0;

    downloadQueue.clear();

    if (cancelled) {
        log.info("Cancelled!");
    } else if (!result) {
        log.error("Downloading failed!");
    } else {
        log.info("Downloading completed!");
    }

    emit finished(cancelled, result);
}
