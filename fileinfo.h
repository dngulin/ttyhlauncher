#ifndef FILEINFO_H
#define FILEINFO_H

#include <QString>


class FileInfo
{
public:
    FileInfo();
    FileInfo(const QString & fileName, const QString & fileHash,
             int fileSize, bool mutability);

    QString name;
    QString hash;
    int size;
    bool isMutable;
};

#endif // FILEINFO_H
