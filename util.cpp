#include <QtNetwork>

#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>
#include <quazip5/quacrc32.h>

#include "util.h"
#include "logger.h"
#include "settings.h"

// Based on: http://stackoverflow.com/questions/20734831
QByteArray Util::makeGzip(const QByteArray &data)
{
    // Compress data
    QByteArray compressedData = qCompress(data);

    // Strip the first six bytes
    // (a 4-byte length put on by qCompress and a 2-byte zlib header)
    // and the last four bytes (a zlib integrity check).
    compressedData.remove(0, 6);
    compressedData.chop(4);

    // Make a generic 10-byte gzip header (see RFC 1952)
    QByteArray header;
    QDataStream headerStream(&header, QIODevice::WriteOnly);

    headerStream << quint16(0x1f8b)
                 << quint16(0x0800)
                 << quint16(0x0000)
                 << quint16(0x0000)
                 << quint16(0x000b);

    // Append a four-byte CRC-32 of the uncompressed data
    // Append 4 bytes uncompressed input size modulo 2^32
    QByteArray footer;
    QDataStream footerStream(&footer, QIODevice::WriteOnly);
    footerStream.setByteOrder(QDataStream::LittleEndian);

    QuaCrc32 crc32;
    footerStream << crc32.calculate(data) << quint32( data.size() );

    return header + compressedData + footer;
}

void Util::removeAll(const QString &filePath)
{
    log( QObject::tr("Delete: %1").arg(filePath) );

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
    log( QObject::tr("Unzip archive: %1").arg(zipFilePath) );

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
                log( QObject::tr("Extract: %1").arg( realFile->fileName() ) );

                if ( realFile->open(QIODevice::WriteOnly) )
                {
                    realFile->write( zipFile.readAll() );
                    realFile->close();
                }
                else
                {
                    QString err = realFile->errorString();
                    log( QObject::tr("Extract error! %1").arg(err) );
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

    log( QObject::tr("Running: %1").arg(runString) );
    output = "Output of \'" + runString + "\':\n";

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(command, args);

    if ( !process.waitForStarted() )
    {
        output += "FAILED_TO_START\n";
    }
    log( QObject::tr("Process started.") );

    process.waitForFinished();
    log( QObject::tr("Process finished.") );

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

void Util::log(const QString &text)
{
    Logger::logger()->appendLine(QObject::tr("Util"), text);
}
