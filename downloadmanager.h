#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QtCore>
#include <QtNetwork>

#include "logger.h"

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = 0);
    ~DownloadManager();

    void addEntry(QString url, QString filename, QString displayname, quint64 size);
    void reset();
    void startDownloads();
    quint64 getDownloadsSize();

private:
    quint64 downloadTotal;
    quint64 downloaded;

    QStringList fileNames;
    QStringList displayNames;
    QStringList urls;

    QNetworkAccessManager* nam;
    QNetworkReply* downloadReply;

    Logger* logger;

signals:
    void beginDownloadFile(QString target);
    void error(QString errorString);
    void progressChanged(int progress);
    void finished();

private slots:
    void downloadNextFile();
    void fileProgress(qint64 bytesReceived, qint64 downloadTotal);

};

#endif // DOWNLOADMANAGER_H
