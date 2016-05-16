#include <QtCore>
#include "fileinfo.h"

FileInfo::FileInfo()
{
    qRegisterMetaType<FileInfo>("FileInfo");

    name = "";
    hash = "";
    size = 0;
    isMutable = false;
}

FileInfo::FileInfo(const QString &fileName, const QString &fileHash,
                   int fileSize, bool mutability)
{
    name = fileName;
    hash = fileHash;
    size = fileSize;
    isMutable = mutability;
}
