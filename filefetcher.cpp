#include "filefetcher.h"
#include "settings.h"

#include <QDebug>

FileFetcher::FileFetcher(QObject *parent) :
    QObject(parent)
{
    fetchSize = 0;
    fetched = 0;

    current = 0;

    hasFetchErrors = false;

    fetchReply = NULL;
    nam = Settings::instance()->getNetworkAccessManager();
    logger = Logger::logger();
}

FileFetcher::~FileFetcher()
{
}

void FileFetcher::log(const QString &text)
{
    logger->append( tr("FileFetcher"), text + QString("\n") );
}

void FileFetcher::add(QUrl url, QString filename)
{
    log(tr("Add to download list: ") + filename);
    fetchData.append( QPair<QUrl, QString >(url, filename) );
}

void FileFetcher::add(QUrl url, QString filename, quint64 size)
{
    add(url, filename);
    fetchSize += size;
}

void FileFetcher::reset()
{
    qDebug() << "RESET";

    fetched = 0;
    fetchSize = 0;

    fetchData.clear();

    hasFetchErrors = false;
}

void FileFetcher::cancel()
{
    qDebug() << "CANCEL";

    if ( (fetchReply != NULL) && fetchReply->isRunning() )
    {
        fetchReply->abort();
    }
    reset();
}

// Fetch sizes
void FileFetcher::fetchSizes()
{
    if (fetchData.count() > 0)
    {
        log( tr("Request downloads sizes...") );

        current = 0;
        fetchSize = 0;
        fetchCurrentSize();
    }
    else
    {
        log( tr("Try to get file sizes for empty list") );
        emit sizesFetchFinished();
    }
}

void FileFetcher::fetchCurrentSize()
{
    QUrl url = fetchData[current].first;
    log( tr("Fetch size for ") + url.toString() );

    fetchReply = nam->head( QNetworkRequest(url) );

    connect(fetchReply, &QNetworkReply::finished,
            this, &FileFetcher::updateFetchSize);
}

void FileFetcher::updateFetchSize()
{
    if (fetchReply->error() == QNetworkReply::NoError)
    {
        quint64 fileSize = fetchReply->
                           header(QNetworkRequest::ContentLengthHeader)
                           .toULongLong();

        fetchSize += fileSize;

        float percents = ( float(current) / fetchData.count() ) * 100;
        emit sizesFetchProgress( int(percents) );
    }
    else
    {
        log( tr("Error: ") + fetchReply->errorString() );
        emit sizesFetchError( fetchReply->errorString() );
    }

    fetchReply->close();
    fetchReply->deleteLater();

    current++;
    if ( current <= fetchData.count() )
    {
        fetchCurrentSize();
    }
    else
    {
        current = 0;
        log( tr("Downloading finished.") );
        emit sizesFetchFinished();
    }
}

quint64 FileFetcher::getFetchSize()
{
    return fetchSize;
}

int FileFetcher::getCount()
{
    return fetchData.count();
}

// Fetch files
void FileFetcher::fetchFiles()
{
    if (fetchData.count() > 0)
    {
        log( tr("Begin downloading files...") );

        current = 0;
        fetchCurrentFile();
    }
    else
    {
        log( tr("Try to download empty file list!") );
        emit filesFetchFinished();
    }
}

void FileFetcher::fetchCurrentFile()
{
    QUrl url = fetchData[current].first;
    QString fname = fetchData[current].second;

    log( tr("Downloading ") + url.toString() + QString(" ...") );
    emit filesFetchNewTarget(url.toString(), fname);

    fetchReply = nam->get( QNetworkRequest(url) );

    qDebug() << "pre connect";
    qDebug() << "open" << fetchReply->isOpen();
    qDebug() << "run"  << fetchReply->isRunning();
    qDebug() << "fin" << fetchReply->isFinished();
    qDebug() << "err" << fetchReply->error();

    // FIXME: No one slot will be invoked. Why?
    // I don't know how to fix this shit...

    connect(fetchReply, &QNetworkReply::downloadProgress,
            this, &FileFetcher::fileFetchProgress);

    connect(fetchReply, &QNetworkReply::finished,
            this, &FileFetcher::saveCurrentFile);

    connect( fetchReply, SIGNAL( error(QNetworkReply::NetworkError) ),
             this, SLOT( fetchError(QNetworkReply::NetworkError) ) );

    connect( fetchReply, SIGNAL( sslErrors(QList<QSslError>)),
             this, SLOT( fetchSslError(QList<QSslError>)) );

    qDebug() << "post connect";
    qDebug() << "open" << fetchReply->isOpen();
    qDebug() << "run"  << fetchReply->isRunning();
    qDebug() << "fin" << fetchReply->isFinished();
    qDebug() << "err" << fetchReply->error();
}

void FileFetcher::fileFetchProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_UNUSED(bytesTotal);

    float baseValue = (float(fetched) / fetchSize) * 100;
    float addValue = (float(bytesReceived) / fetchSize) * 100;

    emit filesFetchProgress( int(baseValue + addValue) );
}

void FileFetcher::fetchError(QNetworkReply::NetworkError error)
{
    qDebug() << "ERR" << error;
}

void FileFetcher::fetchSslError(QList<QSslError> err)
{
    qDebug() << "SSL ERR" << err;
}

void FileFetcher::saveCurrentFile()
{
    qDebug() << "SAVE";

    QString fname = fetchData[current].second;

    if (fetchReply->error() == QNetworkReply::NoError)
    {
        QFile file(fname);
        QDir fdir = QFileInfo(fname).absoluteDir();
        fdir.mkpath( fdir.absolutePath() );

        if ( !file.open(QIODevice::WriteOnly) )
        {
            hasFetchErrors = true;

            log( tr("Error: ") + file.errorString() );
            emit filesFetchError( file.errorString() );
        }
        else
        {
            file.write( fetchReply->readAll() );
            file.close();

            fetched += file.size();

            log( tr("OK") );
            emit filesFetchProgress( int(float(fetched) / fetchSize * 100) );
        }
    }
    else
    {
        hasFetchErrors = true;

        log( tr("Error: ") + fetchReply->errorString() );
        emit filesFetchError( fetchReply->errorString() );
    }

    fetchReply->close();
    fetchReply->deleteLater();

    current++;
    if ( current <= fetchData.count() )
    {
        fetchCurrentFile();
    }
    else
    {
        current = 0;
        log( tr("Downloading finished.") );

        emit filesFetchFinished();
        emit filesFetchResult(hasFetchErrors);
    }
}
