#include "launcherwindow.h"
#include <QApplication>
#include <QtCore>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LauncherWindow w;
    w.show();

    return a.exec();
}
