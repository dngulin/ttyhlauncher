#include <utility>

#include "namedlogger.h"

Ttyh::Logs::NamedLogger::NamedLogger(const QSharedPointer<Logger> &logger, const QString &who)
{
    this->logger = logger;
    this->who = who;
}

void Ttyh::Logs::NamedLogger::info(const QString &msg)
{
    logger->info(who, msg);
}

void Ttyh::Logs::NamedLogger::warning(const QString &msg)
{
    logger->warning(who, msg);
}

void Ttyh::Logs::NamedLogger::error(const QString &msg)
{
    logger->error(who, msg);
}
