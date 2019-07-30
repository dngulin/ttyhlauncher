#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include "filelogger.h"

Ttyh::Logs::FileLogger::FileLogger(const QString &dirName, int logsCount): QObject(nullptr)
{
    auto dataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    auto logsPath = QString("%1/%2").arg(dataPath, dirName);
    QDir().mkpath(logsPath);

    auto logPath = QString("%1/ttyh-launcher.%2.log");
    for (auto i = logsCount - 1; i >= 0; i--) {
        auto currLog = logPath.arg(logsPath, i);

        if (QFile::exists(currLog))
            QFile::remove(currLog);

        if (i == 0) break;

        auto prevLog = logPath.arg(logsPath, i);

        if (QFile::exists(prevLog))
            QFile::rename(prevLog, currLog);
    }

    logFile.setFileName(logPath.arg(logsPath, 0));
    if (!logFile.open(QIODevice::Text | QIODevice::Append | QIODevice::WriteOnly)) {
        QTextStream(stderr) << "Can't open logfile!";
    }
}

void Ttyh::Logs::FileLogger::info(const QString &who, const QString &msg)
{
    log("INFO", who, msg);
}

void Ttyh::Logs::FileLogger::warning(const QString &who, const QString &msg)
{
    log("WARNING", who, msg);
}

void Ttyh::Logs::FileLogger::error(const QString &who, const QString &msg)
{
    log("ERROR", who, msg);
}

void Ttyh::Logs::FileLogger::log(const QString &lvl, const QString &who, const QString &msg)
{
    auto dateTime = QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate);
    raw(QString("%1 [%2] %3: %4").arg(dateTime, lvl, who, msg));
}

void Ttyh::Logs::FileLogger::raw(const QString &line)
{
    if (logFile.isOpen())
    {
        QTextStream stream(&logFile);
        stream.setCodec("UTF-8");
        stream << line << "\n";
    }

    emit onLog(line);
}
