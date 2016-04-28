#include "filefetcher.h"
#include "settings.h"

FileFetcher::FileFetcher(QObject *parent) :
    QObject(parent)
{
    fetchSize = 0;
    fetched = 0;

    current = 0;

    hasFetchErrors = false;

    logger = Logger::logger();

    hiddenLenght = Settings::instance()->getBaseDir().length() + 1;
}

FileFetcher::~FileFetcher()
{
}

void FileFetcher::log(const QString &text)
{
    logger->appendLine( tr("FileFetcher"), text );
}

void FileFetcher::add(QUrl url, QString filename)
{
    fetchData.append( QPair<QUrl, QString >(url, filename) );
}

void FileFetcher::add(QUrl url, QString filename, quint64 size)
{
    add(url, filename);
    fetchSize += size;
}

void FileFetcher::reset()
{
    fetched = 0;
    fetchSize = 0;

    fetchData.clear();

    hasFetchErrors = false;
}

void FileFetcher::cancel()
{
    df.cancel();
    reset();
}

// Fetch sizes
void FileFetcher::fetchSizes()
{
    if (fetchData.count() > 0)
    {
        log( tr("Request downloads sizes...") );

        connect(&df, &DataFetcher::finished, this, &FileFetcher::sizeFetched);

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
    df.makeHead(url);
}

void FileFetcher::sizeFetched(bool result)
{
    if (result)
    {
        fetchSize += df.getSize();

        float percents = ( float(current + 1) / fetchData.count() ) * 100;
        emit sizesFetchProgress( int(percents) );
    }
    else
    {
        emit sizesFetchError( df.errorString() );
    }

    current++;
    if ( current < fetchData.count() )
    {
        fetchCurrentSize();
    }
    else
    {
        disconnect(&df, &DataFetcher::finished,
                   this, &FileFetcher::sizeFetched);

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

void FileFetcher::setHiddenLenght(int len)
{
    hiddenLenght = len;
}

// Fetch files
void FileFetcher::fetchFiles()
{
    if (fetchData.count() > 0)
    {
        log( tr("Begin downloading files...") );

        connect(&df, &DataFetcher::finished, this, &FileFetcher::fileFetched);
        connect(&df, &DataFetcher::progress,
                this, &FileFetcher::fileFetchProgress);

        hasFetchErrors = false;
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

    emit filesFetchNewTarget(url.toString(), fname);

    df.makeGet(url);
}

void FileFetcher::fileFetchProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_UNUSED(bytesTotal);

    float baseValue = (float(fetched) / fetchSize) * 100;
    float addValue = (float(bytesReceived) / fetchSize) * 100;

    emit filesFetchProgress( int(baseValue + addValue) );
}

void FileFetcher::fileFetched(bool result)
{

    QString fname = fetchData[current].second;

    if (result)
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
            file.write( df.getData() );
            file.close();

            fetched += file.size();

            QString shortName = fname.mid( hiddenLenght );
            log( tr("Saved file: ") + shortName );

            emit filesFetchProgress( int(float(fetched) / fetchSize * 100) );
        }
    }
    else
    {
        hasFetchErrors = true;
        emit filesFetchError( df.errorString() );
    }

    current++;
    if ( current < fetchData.count() )
    {
        fetchCurrentFile();
    }
    else
    {
        current = 0;
        log( tr("Downloading finished.") );

        disconnect(&df, &DataFetcher::finished, this,
                   &FileFetcher::fileFetched);

        disconnect(&df, &DataFetcher::progress,
                   this, &FileFetcher::fileFetchProgress);

        emit filesFetchFinished();
        emit filesFetchResult(!hasFetchErrors);
    }
}
