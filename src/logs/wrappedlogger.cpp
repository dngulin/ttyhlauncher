#include <utility>

#include "wrappedlogger.h"

Ttyh::Logs::WrappedLogger::WrappedLogger(const QPointer<FileLogger> &logger, const QString &who)
{
    this->logger = logger;
    this->who = who;
}

void Ttyh::Logs::WrappedLogger::info(const QString &msg)
{
    logger->info(who, msg);
}

void Ttyh::Logs::WrappedLogger::warning(const QString &msg)
{
    logger->warning(who, msg);
}

void Ttyh::Logs::WrappedLogger::error(const QString &msg)
{
    logger->error(who, msg);
}
