#include <QtNetwork>

#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>
#include <quazip5/quacrc32.h>

#include "util.h"
#include "logger.h"
#include "settings.h"

quint64 Util::getFileSize(QString url) {
    QNetworkAccessManager* manager =
            Settings::instance()->getNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl(url));

    QNetworkReply *reply = manager->head(request);
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    reply->deleteLater();
    return reply->header(QNetworkRequest::ContentLengthHeader).toULongLong();
}


Reply Util::makeGet(QString url) {

    Logger::logger()->append("Util", "Make GET: " + url + "\n");

    bool success = true;
    QString errStr;
    QByteArray data;

    QNetworkAccessManager* manager =
            Settings::instance()->getNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    QNetworkReply* reply = manager->get(request);

    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        data.append(reply->readAll());
    } else {

        success = false;

        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            errStr = "Неправильный логин или пароль";
        } else {
            errStr = reply->errorString();
        }
    }

    reply->deleteLater();
    return Reply(success, errStr, data);
}


Reply Util::makePost(QString url, QByteArray postData) {

    Logger::logger()->append("Util", "Make POST: " + url + "\n");

    bool success = true;
    QString errStr;
    QByteArray data;

    QNetworkAccessManager* manager =
            Settings::instance()->getNetworkAccessManager();

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, postData.size());

    QNetworkReply* reply = manager->post(request, postData);

    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        data.append(reply->readAll());
    } else {

        success = false;

        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            errStr = "Неправильный логин или пароль";
        } else {
            errStr = reply->errorString();
        }
    }

    reply->deleteLater();
    return Reply(success, errStr, data);
}

// Realisation from: http://stackoverflow.com/questions/20734831/compress-string-with-gzip-using-qcompress
QByteArray Util::makeGzip(const QByteArray& data) {

    QByteArray compressedData = qCompress(data);
    //  Strip the first six bytes (a 4-byte length put on by qCompress and a 2-byte zlib header)
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
        << quint32(data.size());

    return header + compressedData + footer;
}

void Util::removeAll(QString filePath) {

    Logger::logger()->append("Util", "Removing " + filePath + "\n");

    QFileInfo fileInfo = QFileInfo(filePath);

    if (fileInfo.isDir()) {
        QStringList lstFiles = QDir(filePath).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        if (!lstFiles.isEmpty()) {
            foreach (QString entry, lstFiles)
                removeAll(filePath + "/" + entry);
        }
        QDir(filePath).rmdir(fileInfo.absoluteFilePath());

    } else if (fileInfo.exists()) {
        QFile::remove(filePath);
    }
}

void Util::recursiveFlist(QStringList *list, QString prefix, QString dpath) {

    QDir dir(dpath);
    QStringList fileList = dir.entryList(QDir::Files);
    foreach (QString fname, fileList) {
        list->append(prefix + fname);
    }

    QStringList dirList = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (QString dname, dirList) {
        QDir subdir(dpath + "/" + dname);
        recursiveFlist(list, prefix + dname + "/", dpath + "/" + dname);
    }

}


void Util::unzipArchive(QString zipFilePath, QString extractionPath) {

    Logger* logger = Logger::logger();
    logger->append("Util", "Unzip archive " + zipFilePath + "\n");

    QuaZip zip(zipFilePath);
    if (zip.open(QuaZip::mdUnzip)) {

        QuaZipFile zipFile(&zip);

        for (bool f=zip.goToFirstFile(); f; f=zip.goToNextFile()) {
            zipFile.open(QIODevice::ReadOnly);

            QFile* realFile = new QFile(extractionPath + "/" + zip.getCurrentFileName());

            QFileInfo rfInfo = QFileInfo(*realFile);

            QDir rfDir = rfInfo.absoluteDir();
            rfDir.mkpath(rfDir.absolutePath());

            if (!rfInfo.isDir()) {

                logger->append("Util", "Extracting file " + realFile->fileName() + "\n");

                if (realFile->open(QIODevice::WriteOnly)) {
                    realFile->write(zipFile.readAll());
                    realFile->close();
                } else {
                    logger->append("Util", "Unzip error: " + realFile->errorString() + "\n");
                }
            }

            delete realFile;
            zipFile.close();
        }
        zip.close();
    }

}


QString Util::getCommandOutput(QString command, QStringList args) {

    Logger* logger = Logger::logger();

    QString toRun, result;

    toRun += command;
    toRun += " " + args.join(" ");

    logger->append("Util", "Running: " + toRun + "\n");
    result = "Output of \"" + toRun + "\":\n";

    QProcess* process = new QProcess(0);
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(command, args);

    if (!process->waitForStarted()) result += "FAILED_TO_START\n";
    logger->append("Util", "Process started\n");

    process->waitForFinished();
    logger->append("Util", "Process terminated!\n");

    result += process->readAll();

    delete process;
    return result;
}


QString Util::getFileContetnts(QString path) {
    QString result;
    QFile* file = new QFile(path);
    if (file->open(QIODevice::ReadOnly)) {
        result = file->readAll();
        file->close();
    } else {
        result = "CANT_OPEN_FILE";
    }

    delete file;
    return result;
}


bool Util::downloadFile(QString url, QString fileName) {

    Reply reply = makeGet(url);
    if (!reply.isSuccess()) {
        Logger::logger()->append("Util", "Error: " + reply.getErrorString() + "\n");
        return false;
    } else {
        QFile* file = new QFile(fileName);

        QDir fdir = QFileInfo(fileName).absoluteDir();
        fdir.mkpath(fdir.absolutePath());

        if (file->open(QIODevice::WriteOnly)) {
            file->write(reply.getData());
            file->close();
            delete file;
            return true;
        } else {
            delete file;
            return false;
        }
    }

    return true;
}
