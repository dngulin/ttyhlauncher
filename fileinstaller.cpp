#include "fileinstaller.h"

FileInstaller::FileInstaller()
{
    qRegisterMetaType<QList<InstallInfo> >("QList<InstallInfo>");
}

void FileInstaller::doInstall(const QList<InstallInfo> &list)
{
    cancelled = false;

    int total = list.count();
    int current = 0;

    foreach (const InstallInfo entry, list)
    {
        if (cancelled)
        {
            return;
        }

        processFile(entry);

        current++;
        emit progress( int(float(current) / total * 100) );
    }

    emit finished();
}

void FileInstaller::processFile(const InstallInfo &info)
{
    bool fileExists = QFile::exists(info.path);

    if (info.action == InstallInfo::Delete)
    {
        if (fileExists)
        {
            if ( !QFile::remove(info.path) )
            {
                emit installFailed(info);
            }
        }
    }
    else if (info.action == InstallInfo::Update)
    {
        if (fileExists)
        {
            if ( info.hash.isEmpty() )
            {
                return;
            }

            QCryptographicHash hash(QCryptographicHash::Sha1);
            QFile file(info.path);

            if ( hash.addData(&file) )
            {
                QString sha = QString( hash.result().toHex() );
                if (info.hash == sha)
                {
                    return;
                }
            }

            // Delete exists file before copy
            QFile::remove(info.path);
        }

        QDir dir = QFileInfo(info.path).absoluteDir();
        if ( !dir.exists() )
        {
            dir.mkpath( dir.absolutePath() );
        }

        if ( !QFile::copy(info.srcPath, info.path) )
        {
            emit installFailed(info);
        }
    }
}

void FileInstaller::cancel()
{
    cancelled = true;
}
