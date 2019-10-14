#ifndef ZIP_H
#define ZIP_H

#include <QString>
#include <functional>

namespace Ttyh {
namespace Utils {
namespace Zip {
using LogFunc = std::function<void(const QString)>;
bool unzipDir(const QString &zipPath, const QString &destDir, const LogFunc &logError);

}
}
}

#endif // ZIP_H
