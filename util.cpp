#include <QtNetwork>

#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>
#include <quazip5/quacrc32.h>

#include "util.h"
#include "logger.h"
#include "settings.h"

Reply Util::makeGet(QNetworkAccessManager *nam, const QString &url)
{
    Logger::logger()->appendLine("Util", "Make GET: " + url + "\n");

    bool success = true;
    QString errStr;
    QByteArray data;

    QNetworkRequest request;
    request.setUrl( QUrl(url) );
    QNetworkReply *reply = nam->get(request);

    QEventLoop loop;
    QObject::connect( reply, SIGNAL( finished() ), &loop, SLOT( quit() ) );
    loop.exec();

    if (reply->error() == QNetworkReply::NoError)
    {
        data.append( reply->readAll() );
    }
    else
    {
        success = false;

        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
        {
            errStr = "Bad login";
        }
        else
        {
            errStr = reply->errorString();
        }
    }

    reply->close();
    delete reply;

    return Reply(success, errStr, data);
}

Reply Util::makePost(QNetworkAccessManager *nam, const QString &url,
                     const QByteArray &postData)
{
    Logger::logger()->appendLine("Util", "Make POST: " + url + "\n");

    bool success = true;
    QString errStr;
    QByteArray data;

    QNetworkRequest request;
    request.setUrl( QUrl(url) );
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader( QNetworkRequest::ContentLengthHeader, postData.size() );

    QNetworkReply *reply = nam->post(request, postData);

    QEventLoop loop;
    QObject::connect( reply, SIGNAL( finished() ), &loop, SLOT( quit() ) );
    loop.exec();

    if (reply->error() == QNetworkReply::NoError)
    {
        data.append( reply->readAll() );
    }
    else
    {
        success = false;

        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
        {
            errStr = "Bad login";
        }
        else
        {
            errStr = reply->errorString();
        }
    }

    reply->close();
    delete reply;

    return Reply(success, errStr, data);
}

// Realisation from: http://stackoverflow.com/questions/20734831/compress-string-with-gzip-using-qcompress
QByteArray Util::makeGzip(const QByteArray &data)
{
    QByteArray compressedData = qCompress(data);
    // Strip the first six bytes (a 4-byte length put on by qCompress and a 2-byte zlib header)
    // and the last four bytes (a zlib integrity check).
    compressedData.remove(0, 6);
    compressedData.chop(4);

    QByteArray header;
    QDataStream ds1(&header, QIODevice::WriteOnly);
    // Prepend a generic 10-byte gzip header (see RFC 1952),
    ds1 << quint16(0x1f8b)
        << quint16(0x0800)
        << quint16(0x0000)
        << quint16(0x0000)
        << quint16(0x000b);

    // Append a four-byte CRC-32 of the uncompressed data
    // Append 4 bytes uncompressed input size modulo 2^32
    QByteArray footer;
    QDataStream ds2(&footer, QIODevice::WriteOnly);
    ds2.setByteOrder(QDataStream::LittleEndian);
    QuaCrc32 crc32;
    ds2 << crc32.calculate(data)
        << quint32( data.size() );

    return header + compressedData + footer;
}

void Util::removeAll(const QString &filePath)
{
    Logger::logger()->appendLine("Util", "Removing " + filePath + "\n");

    QFileInfo fileInfo = QFileInfo(filePath);

    if ( fileInfo.isDir() )
    {
        QStringList lstFiles = QDir(filePath).entryList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        if ( !lstFiles.isEmpty() )
        {
            foreach (QString entry, lstFiles)
            {
                removeAll(filePath + "/" + entry);
            }
        }
        QDir(filePath).rmdir( fileInfo.absoluteFilePath() );
    }
    else if ( fileInfo.exists() )
    {
        QFile::remove(filePath);
    }
}

void Util::unzipArchive(const QString &zipFilePath,
                        const QString &extractionPath)
{
    Logger *logger = Logger::logger();
    logger->appendLine("Util", "Unzip archive " + zipFilePath + "\n");

    QuaZip zip(zipFilePath);
    if ( zip.open(QuaZip::mdUnzip) )
    {
        QuaZipFile zipFile(&zip);

        for ( bool f = zip.goToFirstFile(); f; f = zip.goToNextFile() )
        {
            zipFile.open(QIODevice::ReadOnly);

            QFile *realFile = new QFile(
                extractionPath + "/" + zip.getCurrentFileName() );

            QFileInfo rfInfo = QFileInfo(*realFile);

            QDir rfDir = rfInfo.absoluteDir();
            rfDir.mkpath( rfDir.absolutePath() );

            if ( !rfInfo.isDir() )
            {
                logger->appendLine("Util",
                                   "Extracting file " + realFile->fileName()
                                   + "\n");

                if ( realFile->open(QIODevice::WriteOnly) )
                {
                    realFile->write( zipFile.readAll() );
                    realFile->close();
                }
                else
                {
                    logger->appendLine("Util",
                                       "Unzip error: " + realFile->errorString()
                                       + "\n");
                }
            }

            delete realFile;
            zipFile.close();
        }
        zip.close();
    }
}

QString Util::getCommandOutput(const QString &command, const QStringList &args)
{
    QString runString, output;

    runString += command;
    runString += " " + args.join(" ");

    log(QObject::tr("Running: ") + runString);
    output = "Output of \'" + runString + "\':\n";

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(command, args);

    if ( !process.waitForStarted() )
    {
        output += "FAILED_TO_START\n";
    }
    log( QObject::tr("Process started") );

    process.waitForFinished();
    log( QObject::tr("Process finished") );

    output += process.readAll();

    return output;
}

QString Util::getFileContetnts(const QString &path)
{
    QFile file(path);

    if ( file.open(QIODevice::ReadOnly) )
    {
        QString content = file.readAll();
        file.close();

        return content;
    }

    return QString("CANT_OPEN_FILE");
}

bool Util::downloadFile(QNetworkAccessManager *nam, const QString &url,
                        const QString &fileName)
{
    Reply reply = makeGet(nam, url);
    if ( !reply.isSuccess() )
    {
        Logger::logger()->appendLine("Util",
                                     "Error: " + reply.getErrorString() + "\n");
        return false;
    }
    else
    {
        QFile *file = new QFile(fileName);

        QDir fdir = QFileInfo(fileName).absoluteDir();
        fdir.mkpath( fdir.absolutePath() );

        if ( file->open(QIODevice::WriteOnly) )
        {
            file->write( reply.getData() );
            file->close();
            delete file;
            return true;
        }
        else
        {
            delete file;
            return false;
        }
    }

    return true;
}

void Util::log(const QString &text)
{
    Logger::logger()->appendLine(QObject::tr("Util"), text);
}
