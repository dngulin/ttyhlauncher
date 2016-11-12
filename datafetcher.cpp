#include "datafetcher.h"
#include "settings.h"

DataFetcher::DataFetcher(QObject *parent) : QObject(parent)
{
    waiting = false;
    reset();

    nam = Settings::instance()->getNetworkAccessManager();
    logger = Logger::logger();
}

void DataFetcher::log(const QString &text)
{
    logger->appendLine(tr("DataFetcher"), text);
}

void DataFetcher::reset()
{
    size = 0;
    data.clear();
    error.clear();

    if (waiting)
    {
        unhandleReply();
    }
}

void DataFetcher::handleReply()
{
    connect(reply, &QNetworkReply::finished,
            this, &DataFetcher::requestFinished);

    connect(reply, &QNetworkReply::downloadProgress,
            this, &DataFetcher::fetchProgress);

    waiting = true;
}

void DataFetcher::unhandleReply()
{
    disconnect(reply, &QNetworkReply::finished,
               this, &DataFetcher::requestFinished);

    disconnect(reply, &QNetworkReply::downloadProgress,
               this, &DataFetcher::fetchProgress);

    waiting = false;
    reply->deleteLater();
}

const QByteArray &DataFetcher::getData() const
{
    return data;
}

quint64 DataFetcher::getSize()
{
    return size;
}

const QString &DataFetcher::errorString() const
{
    return error;
}

void DataFetcher::makeHead(const QUrl &url)
{
    log( tr("Make HEAD request: %1").arg( url.toString() ) );

    reset();
    reply = nam->head( QNetworkRequest(url) );
    handleReply();
}

void DataFetcher::makeGet(const QUrl &url)
{
    log( tr("Make GET request: %1").arg( url.toString() ) );

    reset();
    reply = nam->get( QNetworkRequest(url) );
    handleReply();
}

void DataFetcher::makePost(const QUrl &url, const QByteArray &postData)
{
    log( tr("Make POST request: %1").arg( url.toString() ) );

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader( QNetworkRequest::ContentLengthHeader, postData.size() );

    reset();
    reply = nam->post(request, postData);
    handleReply();
}

bool DataFetcher::isWaiting() const
{
    return waiting;
}

void DataFetcher::requestFinished()
{
    bool result = true;

    if (reply->error() == QNetworkReply::NoError)
    {
        data = reply->readAll();

        QNetworkRequest::KnownHeaders cl = QNetworkRequest::ContentLengthHeader;
        size = reply->header(cl).toULongLong();
    }
    else
    {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
        {
            error = tr("Bad login.");
        }
        else
        {
            error = reply->errorString();
        }

        result = false;
        log( tr("Error! %1").arg(error) );
    }

    unhandleReply();

    emit finished(result);
}

void DataFetcher::fetchProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit progress(bytesReceived, bytesTotal);
}

void DataFetcher::cancel()
{
    reset();
}
