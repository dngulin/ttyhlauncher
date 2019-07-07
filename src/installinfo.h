#ifndef INSTALLINFO_H
#define INSTALLINFO_H

#include <QString>

class InstallInfo
{
public:
    InstallInfo();

    enum InstallAction { Update, Delete };

    QString path;
    QString srcPath;
    QString hash;
    InstallAction action;
};

#endif // INSTALLINFO_H
