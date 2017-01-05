#ifndef HASHCHECKER_H
#define HASHCHECKER_H

#include <QtCore>

#include "fileinfo.h"

class HashChecker : public QObject
{
    Q_OBJECT

public:
    HashChecker();
    void cancel();

    static QString getDataHash(const QByteArray &data);
    static QString getFileHash(const QString &path);
    static bool isFileHashValid(const QString &path, const QString &hash);

public slots:
    void checkFiles(const QList< FileInfo > &list, bool stopOnBad);

private:
    bool checkFile(const FileInfo fileInfo) const;

    bool cancelled;

signals:
    void progress(int percents);
    void verificationFailed(const FileInfo fileInfo);
    void finished();
};

#endif // HASHCHECKER_H
