#include "oldlogger.h"
#include "oldsettings.h"

#include <QTime>
#include <QDate>
#include <QTextStream>

OldLogger *OldLogger::myInstance = NULL;
OldLogger *OldLogger::logger()
{
    if (myInstance == NULL)
    {
        myInstance = new OldLogger();
    }
    return myInstance;
}

OldLogger::OldLogger(QObject *parent) :
    QObject(parent)
{
    // Rotate logs
    int rotations = 3;

    QString baseDir = OldSettings::instance()->getBaseDir();
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
    QString version = OldSettings::instance()->launcherVersion;

    QString msg = tr("%1, ttyhlauncher-%2 started.").arg(date).arg(version);

    appendLine(tr("Logger"), msg);
}

void OldLogger::appendLine(const QString &sender, const QString &text)
{
    QString time = QTime::currentTime().toString("hh:mm:ss");
    QString prefix = "[" + time + "] " + sender + " >> ";

    QTextStream(stdout) << prefix << text << "\n";

    if ( logFile.isOpen() )
    {
        QTextStream logStream(&logFile);
        logStream.setCodec("UTF-8");

        logStream << prefix << text << "\n";
    }

    emit lineAppended(prefix + text);
}
