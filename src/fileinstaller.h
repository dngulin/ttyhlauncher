#ifndef FILEINSTALLER_H
#define FILEINSTALLER_H

#include <QtCore>
#include "installinfo.h"

class FileInstaller : public QObject
{
    Q_OBJECT

public:
    FileInstaller();
    void cancel();

public slots:
    void doInstall(const QList< InstallInfo > &list);

private:
    bool cancelled;
    void processFile(const InstallInfo &info);

signals:
    void progress(int percents);
    void installFailed(const InstallInfo &installInfo);
    void finished();
};

#endif // FILEINSTALLER_H
