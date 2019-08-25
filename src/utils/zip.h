#ifndef ZIP_H
#define ZIP_H

#include <QString>

namespace Ttyh {
namespace Utils {
namespace Zip {

bool unzipDir(QString zipPath, const QString& destDir);

}
}
}

#endif // ZIP_H
