#include "logger.h"
#include "settings.h"

#include <QTime>
#include <QDate>
#include <QTextStream>

Logger *Logger::myInstance = 0;
Logger *Logger::logger()
{
    if (myInstance == 0)
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

    // remove latest index
    QFile::remove(
        Settings::instance()->getBaseDir() + "/launcher."
        + QString::number(rotations - 1) + ".log");

    // rotate logs
    for (int i = rotations - 2; i >= 0; i--)
    {
        QFile::rename(
            Settings::instance()->getBaseDir() + "/launcher."
            + QString::number(i) + ".log",
            Settings::instance()->getBaseDir() + "/launcher."
            + QString::number(i + 1) + ".log");
    }

    QDir().mkpath( Settings::instance()->getBaseDir() );

    QString fname = Settings::instance()->getBaseDir() + "/launcher.0.log";
    logFile.setFileName(fname);

    if ( !logFile.open(mode) )
    {
        qCritical() << "Can't setup logger!";
    }

    appendLine("Logger", QDate::currentDate().toString("dd.MM.yy")
           + " ttyhlauncher-" + Settings::instance()->launcherVersion
           + " started.");
}

void Logger::appendLine(const QString &sender, const QString &text)
{
    QString prefix = "(" + QTime::currentTime().toString("hh:mm:ss") + ") "
                     + sender + " >> ";

    QTextStream(stdout) << prefix << text << "\n";

    if (logFile.isOpen())
    {
        QTextStream(&logFile) << prefix << text << "\n";
    }

    emit lineAppended( prefix + text );
}
