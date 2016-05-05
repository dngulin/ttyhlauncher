#ifndef UTIL_H
#define UTIL_H

#include <QtCore>
#include <QNetworkAccessManager>
#include "reply.h"

class Util
{
public:

    // Deperecated
    static Reply makeGet(QNetworkAccessManager *nam, const QString &url);
    static Reply makePost(QNetworkAccessManager *nam, const QString &url,
                          const QByteArray &postData);
    static bool downloadFile(QNetworkAccessManager *nam, const QString &url,
                             const QString &fileName);


    // Usable
    static QByteArray makeGzip(const QByteArray &data);

    static QString getCommandOutput(const QString &command,
                                    const QStringList &args);

    static QString getFileContetnts(const QString &path);

    static void removeAll(const QString &filePath);

    static void unzipArchive(const QString &zipFilePath,
                             const QString &extractionPath);

private:
    static void log(const QString &text);
};

#endif // UTIL_H
