#ifndef FILECHECKER_H
#define FILECHECKER_H

#include <QtCore/QObject>
#include <QtCore/QFutureWatcher>
#include "logs/logger.h"
#include "logs/namedlogger.h"
#include "fileinfo.h"

namespace Ttyh {
namespace Storage {
class FileChecker : public QObject
{
    Q_OBJECT
public:
    explicit FileChecker(const QString &dirName, const QSharedPointer<Logs::Logger> &logger);

    void start(const QList<FileInfo> &files);
    void cancel();

signals:
    void rangeChanged(int min, int max);
    void progressChanged(int currentId, const QString &currentName);
    void finished(bool canceled, const QList<FileInfo> &filteredFiles);

private:
    const int prefixLength;
    Logs::NamedLogger log;

    QList<FileInfo> checkingFiles;
    QFutureWatcher<FileInfo> watcher;

    static bool isDownloadRequired(const FileInfo &info);
};
}
}

#endif // FILECHECKER_H
