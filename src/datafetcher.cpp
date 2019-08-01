#include "datafetcher.h"
#include "oldsettings.h"

DataFetcher::DataFetcher(QObject *parent) : QObject(parent)
{
    waiting = false;
    reset();

    nam = OldSettings::instance()->getNetworkAccessManager();
    logger = OldLogger::logger();
    timer = new QTimer(this);
}

DataFetcher::~DataFetcher()
{
    delete timer;
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
    timer->setSingleShot(true);

    connect(timer, &QTimer::timeout,
            this, &DataFetcher::onTimeout);

    timer->start(OldSettings::timeout);

    connect(reply, &QNetworkReply::readyRead,
            this, &DataFetcher::stopTimer);

    connect(reply, &QNetworkReply::finished,
            this, &DataFetcher::onRequestFinished);

    connect(reply, &QNetworkReply::downloadProgress,
            this, &DataFetcher::onDownloadProgress);

    waiting = true;
}

void DataFetcher::unhandleReply()
{
    stopTimer();

    disconnect(timer, &QTimer::timeout,
               this, &DataFetcher::onTimeout);

    disconnect(reply, &QNetworkReply::readyRead,
               this, &DataFetcher::stopTimer);

    disconnect(reply, &QNetworkReply::finished,
               this, &DataFetcher::onRequestFinished);

    disconnect(reply, &QNetworkReply::downloadProgress,
               this, &DataFetcher::onDownloadProgress);

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

void DataFetcher::onRequestFinished()
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

void DataFetcher::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    stopTimer();
    emit progress(bytesReceived, bytesTotal);
}

void DataFetcher::cancel()
{
    reset();
}

void DataFetcher::onTimeout()
{
    if (waiting)
    {
        log( tr("Error! Connection timed out!") );
        reply->abort();
    }
}

void DataFetcher::stopTimer()
{
    if ( timer->isActive() )
    {
        timer->stop();
    }
}
