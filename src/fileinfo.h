#ifndef FILEINFO_H
#define FILEINFO_H

#include <QString>
#include <QUrl>

class FileInfo
{
public:
    OldFileInfo();
    OldFileInfo(const QString & fileName, const QString & fileHash,
             int fileSize, bool mutability);

    QString name;
    QString hash;
    int size;
    bool isMutable;
    QUrl url;
};

#endif // FILEINFO_H
