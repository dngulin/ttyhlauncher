#include "util.h"
#include "logger.h"

#include <QtCore>
#include <QtNetwork>

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

Reply Util::makeGet(QString url) {

    Logger::logger()->append("Util", "Make GET: " + url + "\n");

    bool success = true;
    QString errStr;
    QByteArray data;

    QNetworkAccessManager* manager = new QNetworkAccessManager();
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

    delete manager;
    return Reply(success, errStr, data);
}


Reply Util::makePost(QString url, QByteArray postData) {

    Logger::logger()->append("Util", "Make POST: " + url + "\n");

    bool success = true;
    QString errStr;
    QByteArray data;

    QNetworkAccessManager* manager = new QNetworkAccessManager();

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

    delete manager;
    return Reply(success, errStr, data);
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
