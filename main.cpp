#include "launcherwindow.h"

#include "logger.h"
#include "settings.h"

#include <QApplication>
#include <QSplashScreen>
#include <QBitmap>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Setup translation
    QTranslator t;
    QDate today = QDate::currentDate();
    if (today.month() == 4 && today.day() == 1)
    {
        t.load(":/translations/koi7.qm");
    }
    else
    {
        t.load(":/translations/ru.qm");
    }
    QApplication::installTranslator(&t);

    // Self-update options for replace and remove old executables
    QCommandLineParser args;

    QCommandLineOption argUpdate("u", "Update path", "path");
    QCommandLineOption argRemove("r", "Remove path", "path");

    args.addOption(argUpdate);
    args.addOption(argRemove);

    args.process(a);

    if ( args.isSet(argUpdate) )
    {
        QString temp = a.applicationFilePath();
        QString orig = args.value(argUpdate);

        QFile origFile(orig);
        for (int i = 0; i < 25; i++)
        {
            if ( origFile.open(QIODevice::ReadWrite) )
            {
                origFile.close();
                break;
            }

            QThread::usleep(10);
        }

        if ( QFile::exists(orig) )
        {
            if ( !QFile::remove(orig) )
            {
                QString title = QApplication::translate(
                    "main", "Update error");
                QString text = QApplication::translate(
                    "main", "Can't remove old instance!");

                QMessageBox::critical(NULL, title, text);
            }
        }

        if ( !QFile::copy(temp, orig) )
        {
            QString title = QApplication::translate("main", "Update error");
            QString text = QApplication::translate(
                "main", "Can't copy new instance!");

            QMessageBox::critical(NULL, title, text);
        }
        else
        {
            if ( QProcess::startDetached(orig, QStringList() << "-r" << temp) )
            {
                return 0;
            }
            else
            {
                QString title = QApplication::translate("main", "Update error");
                QString text = QApplication::translate(
                    "main", "Can't run new instance!");

                QMessageBox::critical(NULL, title, text);

                return -1;
            }
        }
    }
    else if ( args.isSet(argRemove) )
    {
        QString temp = args.value(argRemove);

        QFile tempFile(temp);
        for (int i = 0; i < 25; i++)
        {
            if ( tempFile.open(QIODevice::ReadWrite) )
            {
                tempFile.close();
                break;
            }

            QThread::usleep(10);
        }

        if ( QFile::exists(temp) )
        {
            if ( !QFile::remove(temp) )
            {
                QString title = QApplication::translate(
                    "main", "Update warning");
                QString text = QApplication::translate(
                    "main", "Can't remove temporary instance.");

                QMessageBox::warning(NULL, title, text);
            }
        }
    }

    Settings::instance();
    Logger::logger();

    QPixmap logo(":/resources/logo.png");
    QSplashScreen *splash
        = new QSplashScreen(logo, Qt::FramelessWindowHint | Qt::SplashScreen);

    splash->setMask( logo.mask() );

    splash->show();
    Settings::instance()->updateLocalData();
    splash->close();

    delete splash;

    LauncherWindow w;
    w.show();

    return a.exec();
}
