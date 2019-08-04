#ifndef FILEINFO_H
#define FILEINFO_H

#include <QtCore/QString>

namespace Ttyh {
namespace Storage {
struct FileInfo {
    QString url;
    QString path;
    QString hash;
    int size;
};
}
}

#endif // FILEINFO_H
