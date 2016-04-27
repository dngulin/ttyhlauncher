#include "hashchecker.h"

HashChecker::HashChecker()
{
    qRegisterMetaType<QList<FileInfo>>("QList<FileInfo>");
}

void HashChecker::checkFiles(const QList<FileInfo> &list)
{
    cancelled = false;

    int total = list.count();
    int current = 0;

    foreach (const FileInfo entry, list)
    {
        if (cancelled)
        {
            break;
        }

        current++;
        emit progress( current / total * 100 );

        if ( !hashIsValid(entry) )
        {
            emit verificationFailed(entry);
        }
    }

    if (!cancelled)
    {
        emit finished();
    }
}

void HashChecker::cancel()
{
    cancelled = true;
}

bool HashChecker::hashIsValid(const FileInfo fileInfo) const
{
    if ( !QFile::exists(fileInfo.name) )
    {
        return false;
    }
    else if (!fileInfo.isMutable)
    {
        QFile file(fileInfo.name);
        if ( !file.open(QIODevice::ReadOnly) )
        {
            return false;
        }
        else
        {
            QByteArray data = file.readAll();
            file.close();

            QCryptographicHash::Algorithm alg = QCryptographicHash::Sha1;
            QByteArray sha = QCryptographicHash::hash(data, alg).toHex();

            if ( fileInfo.hash != QString(sha) )
            {
                return false;
            }
        }
    }

    return true;
}
