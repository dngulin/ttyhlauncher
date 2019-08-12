#ifndef PLATFORM_H
#define PLATFORM_H

#include "json/libraryinfo.h"

namespace Ttyh {
namespace Utils {
namespace Platform {
bool isLibraryAllowed(const Json::LibraryInfo &libInfo);
QString getLibraryPath(const Json::LibraryInfo &libInfo);

QString getOsName();
QString getOsVersion();
QString getWordSize();

QChar getClassPathSeparator();
}
}
}

#endif // PLATFORM_H
