#ifndef HASHCHECKER_H
#define HASHCHECKER_H

#include <QtCore>

#include "fileinfo.h"

class HashChecker : public QObject
{
    Q_OBJECT

public:
    HashChecker();
    void cancel();

public slots:
    void checkFiles(const QList< FileInfo > &list);

private:
    bool hashIsValid(const FileInfo fileInfo) const;

    bool cancelled;
    mutable QMutex mutex;

    void setCancelled(bool state);
    bool isCancelled() const;

signals:
    void progress( int percents );
    void verificationFailed(const FileInfo fileInfo);
    void finished();
};

#endif // HASHCHECKER_H
