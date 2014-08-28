#include "launcherwindow.h"

#include "logger.h"
#include "settings.h"

#include <QApplication>
#include <QtCore>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Logger::logger();
    Settings::instance()->loadClientList();
    LauncherWindow w;
    w.show();

    return a.exec();
}
