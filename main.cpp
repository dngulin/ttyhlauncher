#include "launcherwindow.h"

#include "logger.h"
#include "settings.h"

#include <QApplication>
#include <QSplashScreen>
#include <QBitmap>
#include <QtCore>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Logger::logger();

    QPixmap logo(":/resources/logo.png");
    QSplashScreen* splash = new QSplashScreen(logo, Qt::FramelessWindowHint| Qt::SplashScreen);
    splash->setMask(logo.mask());
    splash->show();

    Settings::instance()->loadClientList();

    splash->close();
    delete splash;

    LauncherWindow w;    
    w.show();

    return a.exec();
}
