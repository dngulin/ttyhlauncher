#ifndef PLATFORM_H
#define PLATFORM_H

#include "json/libraryinfo.h"

namespace Ttyh {
namespace Utils {
namespace Platform {
bool checkRules(const QList<Json::Rule> &rules);

struct LibPathInfo {
    QString path;
    bool isNativeLib;
};

LibPathInfo getLibraryPathInfo(const Json::LibraryInfo &libInfo);

QString getOsName();
QString getOsVersion();
QString getWordSize();
QString getArch();

QChar getClassPathSeparator();
}
}
}

#endif // PLATFORM_H
