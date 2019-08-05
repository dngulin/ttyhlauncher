#ifndef PLATFORM_H
#define PLATFORM_H

#include "json/libraryinfo.h"

namespace Ttyh {
namespace Utils {
namespace Platform {
bool isLibraryAllowed(const Json::LibraryInfo &libInfo);
QString getLibraryPath(const Json::LibraryInfo &libInfo);

QString osName();
QString osVersion();
QString wordSize();
}
}
}

#endif // PLATFORM_H
