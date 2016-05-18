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

        if ( !hashIsValid(entry) )
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
