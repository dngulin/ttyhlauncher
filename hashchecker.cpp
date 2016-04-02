#include "hashchecker.h"

void HashChecker::checkFiles(const QList<QPair<QString, QString> > &list)
{
    cancelled = false;

    int total = list.count();
    int current = 0;

    QPair<QString, QString> entry;
    foreach (entry, list)
    {
        if (cancelled)
        {
            break;
        }

        QString name = entry.first;
        QString hash = entry.second;

        current++;
        emit progress( current / total * 100 );

        if ( !checkFile(name, hash) )
        {
            emit verificationFailed(name);
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

bool HashChecker::checkFile(const QString &name, const QString &hash)
{
    if ( !QFile::exists(name) )
    {
        return false;
    }
    else if (hash != "mutable")
    {
        QFile file(name);
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

            if ( hash != QString(sha) )
            {
                return false;
            }
        }
    }

    return true;
}
