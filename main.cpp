#include "launcherwindow.h"

#include "logger.h"
#include "settings.h"

#include <unistd.h>

#include <QApplication>
#include <QSplashScreen>
#include <QBitmap>
#include <QMessageBox>

QString tr(const char *str)
{
    return QApplication::translate("main", str);
}

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

        if ( QFile::exists(orig) )
        {
            if ( !QFile::remove(orig) )
            {
                QString title = tr("Update error");
                QString text = tr("Can't remove old instance!");

                QMessageBox::critical(NULL, title, text);
            }
        }

        if ( !QFile::copy(temp, orig) )
        {
            QString title = tr("Update error");
            QString text = tr("Can't copy new instance!");

            QMessageBox::critical(NULL, title, text);
        }
        else
        {
            const char *run = orig.toLocal8Bit().data();
            const char *rem = temp.toLocal8Bit().data();

            if (execlp(run, run, "-r", rem, NULL) == -1)
            {
                QString title = tr("Update error");
                QString text = tr("Can't run new instance!");

                QMessageBox::critical(NULL, title, text);
                return -1;
            }
        }
    }
    else if ( args.isSet(argRemove) )
    {
        QString temp = args.value(argRemove);

        if ( QFile::exists(temp) )
        {
            if ( !QFile::remove(temp) )
            {
                QString title = tr("Update warning");
                QString text = tr("Can't remove temporary instance.");

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
