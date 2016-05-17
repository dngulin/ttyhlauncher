#ifndef UTIL_H
#define UTIL_H

#include <QtCore>

class Util
{
public:
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
