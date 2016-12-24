#ifndef DATAFETCHER_H
#define DATAFETCHER_H

#include <QtCore>
#include <QtNetwork>

#include "logger.h"

class DataFetcher : public QObject
{
    Q_OBJECT
public:
    explicit DataFetcher(QObject *parent = 0);
    ~DataFetcher();

    void makeHead(const QUrl &url);
    void makeGet(const QUrl &url);
    void makePost(const QUrl &url, const QByteArray &postData);

    bool isWaiting() const;

    const QByteArray &getData() const;
    quint64 getSize();
    const QString &errorString() const;

private:
    QNetworkAccessManager *nam;
    QNetworkReply *reply;
    QTimer *timer;

    bool waiting;

    QByteArray data;
    QString error;
    quint64 size;

    Logger *logger;
    void log(const QString &text);

    void reset();
    void handleReply();
    void unhandleReply();

signals:
    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void finished(bool result);

public slots:
    void cancel();

private slots:
    void onTimeout();
    void stopTimer();

    void onRequestFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // DATAFETCHER_H
