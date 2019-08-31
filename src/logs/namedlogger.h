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
    NamedLogger(QSharedPointer<Logger> log, QString who);

    void info(const QString &msg);
    void warning(const QString &msg);
    void error(const QString &msg);

private:
    QSharedPointer<Logger> log;
    const QString who;
};
}
}

#endif //NAMEDLOGGER_H
