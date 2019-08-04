#include "launcherwindow.h"

#include "oldlogger.h"
#include "oldsettings.h"

#include <QApplication>
#include <QSplashScreen>
#include <QBitmap>
#include <QMessageBox>

#include <QtCore/QSharedPointer>
#include "config.h"
#include "logs/logger.h"
#include "logs/namedlogger.h"
#include "settings/settingsmanager.h"
#include "versions/versionsmanager.h"

using namespace Ttyh::Logs;
using namespace Ttyh::Settings;
using namespace Ttyh::Versions;

using QNam = QNetworkAccessManager;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationVersion(PROJECT_VERSION);
    QApplication::setApplicationName("TtyhLauncher 2");

    const QString storeUrl = "https://ttyh.ru/files/newstore";
    const QString masterUrl = "https://master.ttyh.ru";
    const QString dirName = "ttyhlauncher2";

    const int logCount = 3;

    auto logger = QSharedPointer<Logger>(new Logger(dirName, logCount), &QObject::deleteLater);
    QObject::connect(logger.data(), &Logger::onLog,
                     [](const QString &line) { QTextStream(stdout) << line << endl; });

    auto settings = QSharedPointer<SettingsManager>(new SettingsManager(dirName, logger));
    auto nam = QSharedPointer<QNam>(new QNam, &QObject::deleteLater);

    auto testLogger = NamedLogger(logger, "Test");
    testLogger.info(QApplication::applicationName());
    testLogger.info(QApplication::applicationVersion());

    VersionsManager versions(dirName, storeUrl, nam, logger);

    QEventLoop loop;
    QObject::connect(&versions, &VersionsManager::onFetchPrefixesResult, [&](bool result) {
        loop.quit();

        if (result) {
            testLogger.info("Success!");
        } else {
            testLogger.error("Fail!");
        }
    });

    versions.fetchPrefixes();
    loop.exec();

    testLogger.info("Current prefixes:");
    foreach (auto prefix, versions.getPrefixes()) {
        foreach (auto version, prefix.versions) {
            testLogger.info(QString("%1/%2").arg(prefix.id, version));
        }
    }

    return 0;
}
