#include "launcherwindow.h"

#include "oldlogger.h"
#include "oldsettings.h"

#include <QApplication>
#include <QSplashScreen>
#include <QBitmap>
#include <QMessageBox>

#include <QtCore/QSharedPointer>
#include <logs/logger.h>
#include <logs/namedlogger.h>
#include <settings/settingsmanager.h>

namespace Ttyh {
using namespace Logs;
using namespace Settings;

void test()
{
    const QString dirName = "ttyhlauncher2";
    const int logCount = 3;

    auto logger = QSharedPointer<Logger>(new Logger(dirName, logCount), &QObject::deleteLater);
    QObject::connect(logger.data(), &Logger::onLog,
                     [](const QString &line) { QTextStream(stdout) << line << endl; });

    auto settings = QSharedPointer<SettingsManager>(new SettingsManager(dirName, logger));

    auto testLogger = Logs::NamedLogger(logger, "Test");
    testLogger.info("Hello the logging world!");

    testLogger.warning(settings->data.ticket);
}
}

int main(int argc, char *argv[])
{
    Ttyh::test();

    QApplication a(argc, argv);

    // Setup translation
    QTranslator t;
    QDate today = QDate::currentDate();
    if (today.month() == 4 && today.day() == 1) {
        t.load(":/translations/koi7.qm");
    } else {
        t.load(":/translations/ru.qm");
    }
    QApplication::installTranslator(&t);

    OldSettings::instance();
    OldLogger::logger();

#ifdef Q_OS_WIN
    QCommandLineParser args;

    QCommandLineOption argUpdate("u", "Update path", "path");
    QCommandLineOption argRemove("r", "Remove path", "path");

    args.addOption(argUpdate);
    args.addOption(argRemove);

    args.process(a);

    QString who = QApplication::translate("main", "Launcher");

    if (args.isSet(argUpdate)) {
        QString temp = a.applicationFilePath();
        QString orig = args.value(argUpdate);

        QString msg = QApplication::translate("main", "Updating instance: %1");
        Logger::logger()->appendLine(who, msg.arg(orig));

        QFile origFile(orig);
        for (int i = 0; i < 25; i++) {
            if (origFile.open(QIODevice::ReadWrite)) {
                origFile.close();
                break;
            }

            QThread::usleep(10);
        }

        if (QFile::exists(orig)) {
            if (!QFile::remove(orig)) {
                QString title = QApplication::translate("main", "Update error");
                QString text = QApplication::translate("main", "Can't remove old instance!");

                Logger::logger()->appendLine(who, text);

                QMessageBox::critical(NULL, title, text);
            }
        }

        if (!QFile::copy(temp, orig)) {
            QString title = QApplication::translate("main", "Update error");
            QString text = QApplication::translate("main", "Can't copy new instance!");

            Logger::logger()->appendLine(who, text);

            QMessageBox::critical(NULL, title, text);
        } else {
            if (QProcess::startDetached(orig, QStringList() << "-r" << temp)) {
                return 0;
            } else {
                QString title = QApplication::translate("main", "Update error");
                QString text = QApplication::translate("main", "Can't run new instance!");

                Logger::logger()->appendLine(who, text);

                QMessageBox::critical(NULL, title, text);

                return -1;
            }
        }
    } else if (args.isSet(argRemove)) {
        QString temp = args.value(argRemove);

        QString msg = QApplication::translate("main", "Removing instance: %1");
        Logger::logger()->appendLine(who, msg.arg(temp));

        QFile tempFile(temp);
        for (int i = 0; i < 25; i++) {
            if (tempFile.open(QIODevice::ReadWrite)) {
                tempFile.close();
                break;
            }

            QThread::usleep(10);
        }

        if (QFile::exists(temp)) {
            if (!QFile::remove(temp)) {
                QString title = QApplication::translate("main", "Update warning");
                QString text = QApplication::translate("main", "Can't remove temporary instance.");

                Logger::logger()->appendLine(who, text);

                QMessageBox::warning(NULL, title, text);
            }
        }
    }
#endif

    QPixmap logo(":/resources/logo.png");
    QSplashScreen *splash = new QSplashScreen(logo, Qt::FramelessWindowHint | Qt::SplashScreen);
    splash->setMask(logo.mask());
    splash->show();

    OldSettings::instance()->updateLocalData();

#ifdef Q_OS_WIN
    Settings::instance()->fetchLatestVersion();
#endif

    splash->close();
    delete splash;

    LauncherWindow w;
    w.show();

    return a.exec();
}
