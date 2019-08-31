#ifndef LOGGER_H
#define LOGGER_H

#include <QtCore/QObject>
#include <QtCore/QFile>

namespace Ttyh {
namespace Logs {
class Logger : public QObject
{
    Q_OBJECT
public:
    Logger(const QString &workDir, int logsCount);

    void info(const QString &who, const QString &msg);
    void warning(const QString &who, const QString &msg);
    void error(const QString &who, const QString &msg);

    void raw(const QString &line);

signals:
    void onLog(const QString &line) const;

private:
    void log(const QString &lvl, const QString &who, const QString &msg);
    QFile logFile;
};
}
}

#endif //LOGGER_H
