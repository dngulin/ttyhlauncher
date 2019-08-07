#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include "utils/network.h"
#include "downloadfilereply.h"

Ttyh::Storage::DownloadFileReply::DownloadFileReply(QNetworkReply *reply, const QString &filePath)
    : QObject(reply), parentReply(reply), file(filePath), hasWriteError(false), cancelled(false)
{
    Utils::Network::createTimeoutTimer(reply);

    connect(reply, &QNetworkReply::readyRead, [=]() {
        if (hasWriteError || cancelled)
            return;

        if (!file.isOpen()) {

            auto dir = QFileInfo(filePath).absoluteDir();
            dir.mkpath(dir.absolutePath());

            if (!file.open(QIODevice::WriteOnly)) {
                hasWriteError = true;
                reply->abort();
                return;
            }
        }

        auto written = file.write(reply->readAll());
        if (written < 0) {
            hasWriteError = true;
            reply->abort();
            return;
        }

        emit bytesWritten(written);
    });

    connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();

        if (file.isOpen())
            file.close();

        bool result = !hasWriteError && reply->error() == QNetworkReply::NoError;
        emit finished(cancelled, result);
    });
}

void Ttyh::Storage::DownloadFileReply::cancel()
{
    cancelled = true;
    parentReply->abort();
}

QString Ttyh::Storage::DownloadFileReply::errorString() const
{
    if (hasWriteError)
        return "Failed to write a file";

    if (parentReply->error() != QNetworkReply::NoError)
        return parentReply->errorString();

    return "No error";
}
