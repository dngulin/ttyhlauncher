#ifndef WRAPPEDLOGGER_H
#define WRAPPEDLOGGER_H


#include <QtCore/QString>
#include <QtCore/QPointer>
#include "filelogger.h"

namespace Ttyh
{
namespace Logs
{
class WrappedLogger
{
public:
    WrappedLogger(const QPointer<FileLogger> &logger, const QString &who);

    void info(const QString &msg);
    void warning(const QString &msg);
    void error(const QString &msg);

private:
    QString who;
    QPointer<FileLogger> logger;
};
}
}


#endif //WRAPPEDLOGGER_H
