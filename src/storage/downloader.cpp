#include "downloader.h"

using namespace Ttyh::Utils;

Ttyh::Storage::Downloader::Downloader(QString dirName, QString storageUrl,
                                      QSharedPointer<QNetworkAccessManager> nam,
                                      const QSharedPointer<Logs::Logger> &logger)
    : dataPath(std::move(dirName)),
      versionsPath(QString("%1/%2").arg(dataPath, "versions")),
      prefixLength(dataPath.length() + 1),
      storeUrl(std::move(storageUrl)),
      downloading(false),
      currentFileName(""),
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
    currentFileName = "";
    currentSize = 0;
    totalSize = 0;

    for (const auto& fileInfo : files) {
        totalSize += fileInfo.size;
        downloadQueue << fileInfo;
    }

    emit rangeChanged(0, progressRange);
    emit progressChanged(0, currentFileName);

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
    currentFileName = target.path.mid(prefixLength);
    emit progressChanged(getRangedProgress(), currentFileName);

    auto reply = new DownloadFileReply(nam->get(QNetworkRequest(target.url)), target.path);
    currentReply = QPointer<DownloadFileReply>(reply);

    connect(reply, &DownloadFileReply::bytesWritten, [=](int written) {
        if (!downloading)
            return;

        currentSize += written;
        emit progressChanged(getRangedProgress(), currentFileName);
    });

    connect(reply, &DownloadFileReply::finished, [=](bool cancelled, bool result) {
        if (cancelled || !result) {

            if (!result) {
                auto msg = QString("Failed to download '%1': %2");
                log.error(msg.arg(currentFileName, currentReply->errorString()));
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

    currentFileName = "";
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

int Ttyh::Storage::Downloader::getRangedProgress() const
{
    if (totalSize == 0)
        return progressRange / 2; // Just a rough estimation

    return qRound((float(currentSize) / float(totalSize)) * progressRange);
}
