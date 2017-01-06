#include "hashchecker.h"

HashChecker::HashChecker()
{
    qRegisterMetaType<QList<FileInfo> >("QList<FileInfo>");
}

void HashChecker::checkFiles(const QList<FileInfo> &list, bool stopOnBad)
{
    cancelled = false;

    int total = list.count();
    int current = 0;

    foreach (const FileInfo entry, list)
    {
        if (cancelled)
        {
            return;
        }

        current++;
        emit progress( int(float(current) / total * 100) );

        if ( !checkFile(entry) )
        {
            emit verificationFailed(entry);

            if (stopOnBad)
            {
                return;
            }
        }
    }

    emit finished();
}

bool HashChecker::checkFile(const FileInfo fileInfo) const
{
    if ( !QFile::exists(fileInfo.name) )
    {
        return false;
    }

    if (fileInfo.isMutable)
    {
        return true;
    }

    return isFileHashValid(fileInfo.name, fileInfo.hash);
}

void HashChecker::cancel()
{
    cancelled = true;
}

QString HashChecker::getDataHash(const QByteArray &data)
{
    QCryptographicHash sha(QCryptographicHash::Sha1);
    sha.addData(data);
    return QString( sha.result().toHex() );
}

QString HashChecker::getFileHash(const QString &path)
{
    QString hash = "";

    QCryptographicHash sha(QCryptographicHash::Sha1);
    QFile file(path);

    if ( file.open(QIODevice::ReadOnly) )
    {
        if ( sha.addData(&file) )
        {
            hash = QString( sha.result().toHex() );
        }
        file.close();
    }

    return hash;
}

bool HashChecker::isFileHashValid(const QString &path, const QString &hash)
{
    QCryptographicHash sha(QCryptographicHash::Sha1);
    QFile file(path);

    bool shaReady = false;
    if ( file.open(QIODevice::ReadOnly) )
    {
        shaReady = sha.addData(&file);
        file.close();
    }

    if (shaReady)
    {
        QString fileHash( sha.result().toHex() );
        return fileHash.toLower() == hash.toLower();
    }

    return false;
}
