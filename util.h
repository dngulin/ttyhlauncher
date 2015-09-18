#ifndef UTIL_H
#define UTIL_H

#include <QtCore>
#include <QNetworkAccessManager>
#include "reply.h"

namespace Util {

Reply makeGet(QNetworkAccessManager* nam, const QString& url);
Reply makePost(QNetworkAccessManager* nam,
               const QString& url, const QByteArray &postData);
quint64 getFileSize(QNetworkAccessManager *nam, const QString &url);

QByteArray makeGzip(const QByteArray& data);

QString getCommandOutput(QString command, QStringList args);
QString getFileContetnts(QString path);

bool downloadFile(QNetworkAccessManager* nam,
                  const QString& url, const QString &fileName);

void removeAll(QString filePath);
void recursiveFlist(QStringList *list, QString prefix, QString dpath);
void unzipArchive(QString zipFilePath, QString extractionPath);

}

#endif // UTIL_H
