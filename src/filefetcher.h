#ifndef FILEFETCHER_H
#define FILEFETCHER_H

#include <QtCore>
#include <QtNetwork>

#include "oldlogger.h"
#include "datafetcher.h"

class FileFetcher : public QObject
{
    Q_OBJECT

public:
    explicit FileFetcher(QObject *parent = 0);
    ~FileFetcher();

    void add(QUrl url, QString filename);
    void add(QUrl url, QString filename, quint64 size);
    void fetchSizes();
    void fetchFiles();

    quint64 getFetchSize();
    int getCount();

    void setHiddenLenght( int len );

public slots:
    void reset();
    void cancel();

private:
    quint64 fetched;
    quint64 fetchSize;

    QList< QPair<QUrl, QString> > fetchData;
    int current;

    DataFetcher df;
    bool hasFetchErrors;

    OldLogger *logger;
    void log(const QString &text);

    int hiddenLenght;

    bool fetchingSizes;
    bool fetchingFiles;

signals:
    void sizesFetchProgress(int progress);
    void sizesFetchError(QString errorString);
    void sizesFetchFinished();
    void sizesFetchResult(bool result);

    void filesFetchNewTarget(QString url, QString fname);
    void filesFetchError(QString errorString);
    void filesFetchProgress(int progress);
    void filesFetchFinished();
    void filesFetchResult(bool result);

private slots:
    void fetchCurrentSize();
    void sizeFetched(bool result);

    void fetchCurrentFile();
    void fileFetched(bool result);
    void fileFetchProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // FILEFETCHER_H
