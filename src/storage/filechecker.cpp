#include <QtConcurrent/QtConcurrentFilter>
#include <QtCore/QCryptographicHash>
#include <QtCore/QStandardPaths>
#include "filechecker.h"

Ttyh::Storage::FileChecker::FileChecker(const QString &dirName,
                                        const QSharedPointer<Logs::Logger> &logger)
    : prefixLength([&dirName]() {
          auto pattern = QString("%1/%2/");
          auto basePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
          return pattern.arg(basePath, dirName).length();
      }()),
      log(logger, "FileChecker")
{
    connect(&watcher, &QFutureWatcher<FileInfo>::progressRangeChanged,
            [=](int min, int max) { emit rangeChanged(min, max); });

    connect(&watcher, &QFutureWatcher<FileInfo>::progressValueChanged, [=](int fileNumber) {
        auto index = fileNumber - 1;
        if (index >= 0 && index < checkingFiles.count()) {
            emit progressChanged(fileNumber, checkingFiles[index].path.mid(prefixLength));
        } else {
            emit progressChanged(fileNumber, "");
        }
    });

    connect(&watcher, &QFutureWatcher<FileInfo>::finished, [=] { handleFinished(true); });
    connect(&watcher, &QFutureWatcher<FileInfo>::canceled, [=] { handleFinished(false); });
}

void Ttyh::Storage::FileChecker::handleFinished(bool cancelled)
{
    auto filteredFiles = watcher.future().results();
    watcher.setFuture(QFuture<FileInfo>());
    checkingFiles.clear();

    emit finished(cancelled, filteredFiles);
}

void Ttyh::Storage::FileChecker::start(const QList<FileInfo> &files)
{
    log.info("Start checking files...");
    if (watcher.isRunning()) {
        log.error("Failed to start a checking. Already in progress!");
        return;
    }

    checkingFiles = files;

    auto future = QtConcurrent::filtered(files, isDownloadRequired);
    watcher.setFuture(future);
}

void Ttyh::Storage::FileChecker::cancel()
{
    if (watcher.isRunning()) {
        log.info("Wait for cancelling...");
        watcher.cancel();
        watcher.waitForFinished();
        log.info("Task have been cancelled!");
    } else {
        log.warning("Cancellation is ignored. Task is not running!");
    }
}

bool Ttyh::Storage::FileChecker::isDownloadRequired(const FileInfo &info)
{
    QFile file(info.path);

    if (!file.exists())
        return true;

    if (file.size() != info.size)
        return true;

    if (info.hash.isEmpty())
        return false;

    QCryptographicHash sha1(QCryptographicHash::Sha1);

    file.open(QIODevice::ReadOnly);
    sha1.addData(&file);

    return info.hash != sha1.result().toHex();
}
