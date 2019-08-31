#include "namedlogger.h"

Ttyh::Logs::NamedLogger::NamedLogger(QSharedPointer<Logger> log, QString who)
    : log(std::move(log)), who(std::move(who))
{
}

void Ttyh::Logs::NamedLogger::info(const QString &msg)
{
    log->info(who, msg);
}

void Ttyh::Logs::NamedLogger::warning(const QString &msg)
{
    log->warning(who, msg);
}

void Ttyh::Logs::NamedLogger::error(const QString &msg)
{
    log->error(who, msg);
}
