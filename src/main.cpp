#include <QApplication>
#include <QSplashScreen>
#include <QBitmap>
#include <QMessageBox>
#include <QtCore/QSharedPointer>

#include "config.h"
#include "logs/logger.h"
#include "settings/settingsmanager.h"
#include "versions/versionsmanager.h"
#include "profiles/profilesmanager.h"
#include "profiles/profilerunner.h"
#include "master/ttyhclient.h"
#include "storage/filechecker.h"
#include "storage/downloader.h"

using namespace Ttyh::Logs;
using namespace Ttyh::Settings;
using namespace Ttyh::Versions;
using namespace Ttyh::Profiles;
using namespace Ttyh::Master;
using namespace Ttyh::Storage;
using namespace Ttyh::Utils;

template<typename T>
using QSP = QSharedPointer<T>;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationVersion(PROJECT_VERSION);
    QApplication::setApplicationName("TtyhLauncher");

    const QString storeUrl = "https://ttyh.ru/files/newstore";
    const QString masterUrl = "https://master.ttyh.ru";
    const QString dirName = "ttyhlauncher2";

    const int logCount = 3;

    auto qDel = &QObject::deleteLater;
    auto nam = QSP<QNetworkAccessManager>(new QNetworkAccessManager(), qDel);
    auto logger = QSP<Logger>(new Logger(dirName, logCount), qDel);
    QObject::connect(logger.data(), &Logger::onLog,
                     [](const QString &line) { QTextStream(stdout) << line << endl; });

    auto settings = QSP<SettingsManager>(new SettingsManager(dirName, logger));
    auto profiles = QSP<ProfilesManager>(new ProfilesManager(dirName, logger));
    auto versions = QSP<VersionsManager>(new VersionsManager(dirName, storeUrl, nam, logger), qDel);

    auto ticket = settings->data.ticket;
    auto client = QSP<TtyhClient>(new TtyhClient(masterUrl, ticket, nam, logger), qDel);

    auto checker = QSP<FileChecker>(new FileChecker(dirName, logger), qDel);
    auto downloader = QSP<Downloader>(new Downloader(storeUrl, dirName, nam, logger), qDel);
    auto runner = QSP<ProfileRunner>(new ProfileRunner(dirName, logger), qDel);

    return 0;
}
