#include "launcherwindow.h"
#include "logger.h"

#include <QApplication>
#include <QtCore>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Logger::logger();
    LauncherWindow w;
    w.show();

    return a.exec();
}
