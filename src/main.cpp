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
    QApplication::setApplicationName("TtyhLauncher");

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

    QEventLoop fetchPrefixesLoop;
    QObject::connect(&versions, &VersionsManager::onFetchPrefixesResult, [&](bool result) {
        fetchPrefixesLoop.quit();
        testLogger.info("Prefixes fetch result: " + QString(result ? "OK" : "FAIL"));
    });

    versions.fetchPrefixes();
    fetchPrefixesLoop.exec();

    testLogger.info("Known versions:");
    foreach (auto prefix, versions.getPrefixes()) {
        foreach (auto version, prefix.versions) {
            testLogger.info(QString("%1/%2").arg(prefix.id, version));
        }
    }

    QEventLoop fetchIndexesLoop;
    QObject::connect(&versions, &VersionsManager::onFetchVersionIndexesResult, [&](bool result) {
        fetchIndexesLoop.quit();
        testLogger.info("Version indexes fetch result: " + QString(result ? "OK" : "FAIL"));
    });

    auto version = FullVersionId("default", versions.getPrefixes()["default"].latestVersionId);
    versions.fetchVersionIndexes(version);
    fetchIndexesLoop.exec();

    return 0;
}
