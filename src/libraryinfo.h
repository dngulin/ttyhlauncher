#ifndef LIBRARYINFO_H
#define LIBRARYINFO_H

#include <QString>


class LibraryInfo
{
public:
    LibraryInfo();
    LibraryInfo(const QString & libName, bool nativity = false);

    QString name;
    bool isNative;

};

#endif // LIBRARYINFO_H
