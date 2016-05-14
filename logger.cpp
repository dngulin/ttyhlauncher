#include "logger.h"
#include "settings.h"

#include <QTime>
#include <QDate>
#include <QTextStream>

Logger *Logger::myInstance = NULL;
Logger *Logger::logger()
{
    if (myInstance == NULL)
    {
        myInstance = new Logger();
    }
    return myInstance;
}

Logger::Logger(QObject *parent) :
    QObject(parent)
{
    // Rotate logs
    int rotations = 3;

    QString baseDir = Settings::instance()->getBaseDir();
    QString last = QString::number(rotations - 1);

    QFile::remove(baseDir + "/launcher." + last + ".log");

    for (int i = rotations - 2; i >= 0; i--)
    {
        QString cur = QString::number(i);
        QString nxt = QString::number(i + 1);

        QFile::rename(baseDir + "/launcher." + cur + ".log",
                      baseDir + "/launcher." + nxt + ".log");
    }

    // Setup logfile
    QDir().mkpath(baseDir);
    QString logFileName = baseDir + "/launcher.0.log";

    logFile.setFileName(logFileName);

    mode = QIODevice::Text | QIODevice::Append | QIODevice::WriteOnly;

    if ( !logFile.open(mode) )
    {
        qCritical() << "Can't setup logger!";
    }

    QString date = QDate::currentDate().toString("dd.MM.yy");
    QString version = Settings::instance()->launcherVersion;
    QString launcher = date + ", ttyhlauncher-" + version + " ";

    appendLine( tr("Logger"), launcher + tr("started.") );
}

void Logger::appendLine(const QString &sender, const QString &text)
{
    QString time = QTime::currentTime().toString("hh:mm:ss");
    QString prefix = "[" + time + "] " + sender + " >> ";

    QTextStream(stdout) << prefix << text << "\n";

    if ( logFile.isOpen() )
    {
        QTextStream(&logFile) << prefix << text << "\n";
    }

    emit lineAppended(prefix + text);
}
