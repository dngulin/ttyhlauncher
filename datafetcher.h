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

    void makeGet(const QUrl &url);
    void makePost(const QUrl &url, const QByteArray &postData);

    const QByteArray &getData() const;
    const QString &getError() const;

private:
    QNetworkAccessManager *nam;
    QNetworkReply *reply;

    QByteArray data;
    QString error;

    Logger *logger;
    void log(const QString &text);

signals:
    void finished(bool result);

public slots:
    void cancel();

private slots:
    void requestFinished();
};

#endif // DATAFETCHER_H
