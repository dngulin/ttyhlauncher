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

    if (fileInfo.isMutable)
    {
        return true;
    }

    QFile file(fileInfo.name);
    QCryptographicHash hash(QCryptographicHash::Sha1);

    if ( file.open(QIODevice::ReadOnly) )
    {
        bool isValid = false;
        if ( hash.addData(&file) )
        {
            isValid = (QString( hash.result().toHex() ) == fileInfo.hash);
        }
        file.close();
        return isValid;
    }

    return false;
}
