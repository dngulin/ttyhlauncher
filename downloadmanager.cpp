#include "downloadmanager.h"

DownloadManager::DownloadManager(QObject *parent) :
    QObject(parent)
{
    downloadTotal = 0;
    downloaded = 0;

    nam = new QNetworkAccessManager(this);

    logger = Logger::logger();
}

DownloadManager::~DownloadManager()
{
    delete nam;
}

// Methods
void DownloadManager::addEntry(QString url, QString filename, QString displayname, quint64 size) {
    logger->append("DownloadManager", "New target: " + url + "\n");

    fileNames.append(filename);
    displayNames.append(displayname);
    urls.append(url);
    downloadTotal += size;
}

void DownloadManager::reset() {
    logger->append("DownloadManager", "Reset targets\n");

    downloadTotal = 0;
    downloaded = 0;

    fileNames.clear();
    displayNames.clear();
    urls.clear();
}

quint64 DownloadManager::getDownloadsSize() {
    return downloadTotal;
}

void DownloadManager::startDownloads() {
    logger->append("DownloadManager", "Begin download queue...\n");

    emit beginDownloadFile(displayNames.first());
    logger->append("DownloadManager", "Downloading " + urls.first() + "...\n");
    downloadReply = nam->get(QNetworkRequest(QUrl(urls.first())));

    connect(downloadReply, SIGNAL(finished()), this, SLOT(downloadNextFile()));
    connect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(fileProgress(qint64,qint64)));
}

// Slots
void DownloadManager::downloadNextFile() {

    // Save file, or send error
    if (downloadReply->error() == QNetworkReply::NoError) {

        logger->append("DownloadManager", "Try to save " + fileNames.first() + "\n");
        QFile* file = new QFile(fileNames.first());

        QDir fdir = QFileInfo(fileNames.first()).absoluteDir();
        fdir.mkpath(fdir.absolutePath());

        if (!file->open(QIODevice::WriteOnly)) {

            logger->append("DownloadManager", "Error: " + file->errorString() + "\n");
            emit error(file->errorString());

        } else {

            file->write(downloadReply->readAll());
            file->close();
            downloaded += file->size();
            logger->append("DownloadManager", "File saved\n");
            emit progressChanged(int(float(downloaded) / downloadTotal * 100));
        }
        delete file;

    } else {
        logger->append("DownloadManager", "Error: " + downloadReply->errorString() + "\n");
        emit error(downloadReply->errorString());
    }

    // Disconnect old reply connections
    connect(downloadReply, SIGNAL(finished()), this, SLOT(downloadNextFile()));
    connect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(fileProgress(qint64,qint64)));

    // Remove completed entry
    fileNames.removeFirst();
    displayNames.removeFirst();
    urls.removeFirst();

    if (fileNames.isEmpty()) {
        logger->append("DownloadManager", "Download queue is empty\n");
        emit finished();
        return;
    }

    emit beginDownloadFile(displayNames.first());
    logger->append("DownloadManager", "Downloading " + urls.first() + "...\n");
    downloadReply = nam->get(QNetworkRequest(QUrl(urls.first())));

    connect(downloadReply, SIGNAL(finished()), this, SLOT(downloadNextFile()));
    connect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(fileProgress(qint64,qint64)));

}

void DownloadManager::fileProgress(qint64 bytesReceived, qint64 bytesTotal) {

    float baseValue = (float(downloaded) / downloadTotal) * 100;
    float addValue = (float(bytesReceived) / downloadTotal) * 100;

    emit progressChanged(int(baseValue + addValue));
}
