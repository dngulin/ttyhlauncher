#include "logger.h"
#include "settings.h"

#include <QTime>
#include <QDate>
#include <QTextStream>

Logger* Logger::myInstance = 0;
Logger* Logger::logger() {
    if (myInstance == 0) myInstance = new Logger();
    return myInstance;
}

Logger::Logger(QObject *parent) :
    QObject(parent)
{
    logFile = new QFile(Settings::instance()->getBaseDir() + "/launcher.log");

    if (!logFile->open(QIODevice::Append | QIODevice::Text)) qCritical() << "Can't setup logger!";

    append("logger", QDate::currentDate().toString("dd.MM.yy")
           + " ttyhlauncher-" + Settings::instance()->launcherVerion + " started.\n");

}

void Logger::append(QString sender, QString text)
{
    QString prefix = "(" + QTime::currentTime().toString("hh:mm") + ") " + sender + " >> ";

    QTextStream(stdout) << prefix << text;
    QTextStream textout(logFile);
    if (logFile != 0) textout << prefix << text;

}
