#include "logger.h"
#include "settings.h"

#include <QTime>
#include <QDate>
#include <QTextStream>
#include <QApplication>

Logger* Logger::myInstance = 0;
Logger* Logger::logger() {
    if (myInstance == 0) myInstance = new Logger();
    return myInstance;
}

Logger::Logger(QObject *parent) :
    QObject(parent)
{
    // Rotate logs
    int rotations = 3;

    // remove latest index
    QFile::remove(Settings::instance()->getBaseDir() + "/launcher." + QString::number(rotations - 1) + ".log");

    // rotate logs
    for (int i = rotations - 2; i >=0; i--) {
        QFile::rename(Settings::instance()->getBaseDir()+ "/launcher." + QString::number(i) + ".log",
                      Settings::instance()->getBaseDir()+ "/launcher." + QString::number(i + 1) + ".log");
    }

    QDir().mkpath(Settings::instance()->getBaseDir()); // Make dir, if not exist
    logFile = new QFile(Settings::instance()->getBaseDir() + "/launcher.0.log");

    if (!logFile->open(QIODevice::Append | QIODevice::Text)) qCritical() << "Can't setup logger!";

    append("Logger", QDate::currentDate().toString("dd.MM.yy")
           + " ttyhlauncher-" + Settings::instance()->launcherVersion + " started.\n");

}

void Logger::append(QString sender, QString text)
{
    QString prefix = "(" + QTime::currentTime().toString("hh:mm:ss") + ") " + sender + " >> ";

    QTextStream(stdout) << prefix << text;
    QTextStream textout(logFile);
    if (logFile != NULL) textout << prefix << text;

    emit textAppended(prefix + text);
    QApplication::processEvents();
}
