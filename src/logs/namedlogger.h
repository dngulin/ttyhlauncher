#ifndef NAMEDLOGGER_H
#define NAMEDLOGGER_H

#include <QtCore/QString>
#include <QtCore/QSharedPointer>
#include "logger.h"

namespace Ttyh {
namespace Logs {
class NamedLogger
{
public:
    NamedLogger(const QSharedPointer<Logger> &logger, const QString &who);

    void info(const QString &msg);
    void warning(const QString &msg);
    void error(const QString &msg);

private:
    QString who;
    QSharedPointer<Logger> logger;
};
}
}

#endif //NAMEDLOGGER_H
