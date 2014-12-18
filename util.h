#ifndef UTIL_H
#define UTIL_H

#include <QtCore>
#include "reply.h"

namespace Util {

Reply makeGet(QString url);
Reply makePost(QString url, QByteArray postData);
quint64 getFileSize(QString url);

QByteArray makeGzip(const QByteArray& data);

QString getCommandOutput(QString command, QStringList args);
QString getFileContetnts(QString path);

bool downloadFile(QString url, QString fileName);
void removeAll(QString filePath);
void unzipArchive(QString zipFilePath, QString extractionPath);

}

#endif // UTIL_H
