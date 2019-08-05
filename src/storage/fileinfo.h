#ifndef FILEINFO_H
#define FILEINFO_H

#include <QtCore/QString>

namespace Ttyh {
namespace Storage {
struct FileInfo {
    FileInfo() : url(""), path(""), hash(""), size(0) {}
    FileInfo(QString url, QString path, QString hash, int size)
        : url(std::move(url)), path(std::move(path)), hash(std::move(hash)), size(size)
    {
    }

    QString url;
    QString path;
    QString hash;
    int size;
};
}
}

#endif // FILEINFO_H
