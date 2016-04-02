#include "datafetcher.h"
#include "settings.h"

DataFetcher::DataFetcher(QObject *parent) : QObject(parent)
{
    reply = NULL;
    nam = Settings::instance()->getNetworkAccessManager();
    logger = Logger::logger();
}

void DataFetcher::log(const QString &text)
{
    logger->append( tr("DataFetcher"), text + QString("\n") );
}

const QByteArray &DataFetcher::getData() const
{
    return data;
}

const QString &DataFetcher::getError() const
{
    return error;
}

void DataFetcher::makeGet(const QUrl &url)
{
    log( tr("Make GET request: ") + url.toString() );

    reply = nam->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished,
            this, &DataFetcher::requestFinished);
}

void DataFetcher::makePost(const QUrl &url, const QByteArray &postData)
{
    log( tr("Make POST request: ") + url.toString() );

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, postData.size());

    reply = nam->post(request, postData);

    connect(reply, &QNetworkReply::finished,
            this, &DataFetcher::requestFinished);
}

void DataFetcher::requestFinished()
{
    if (reply->error() == QNetworkReply::NoError)
    {
        data = reply->readAll();
        emit finished(true);
    }
    else
    {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
        {
            error = tr( "Bad login" );
        }
        else
        {
           error = reply->errorString();
        }

        log( tr("Error: ") + error );
        emit finished(false);
    }

    reply->close();
    reply->deleteLater();
}

void DataFetcher::cancel()
{
    if ( (reply != NULL) && reply->isRunning() )
    {
        reply->abort();
    }
}
