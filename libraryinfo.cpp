#include "libraryinfo.h"

LibraryInfo::LibraryInfo()
{
    name = "";
    isNative = false;
}

LibraryInfo::LibraryInfo(const QString & libName, bool nativity)
{
    name = libName;
    isNative = nativity;
}

