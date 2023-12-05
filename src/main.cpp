#include <QApplication>
#include <QtCore/QSharedPointer>
#include <QtCore/QTranslator>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

#include "config.h"
#include "launcher.h"
#include "logs/logger.h"
#include "settings/settingsmanager.h"
#include "versions/versionsmanager.h"
#include "profiles/profilesmanager.h"
#include "profiles/profilerunner.h"
#include "master/ttyhclient.h"
#include "storage/filechecker.h"
#include "storage/downloader.h"
#include "news/newsfeed.h"
#include "utils/migrations.h"

using namespace Ttyh;
using namespace Ttyh::Logs;
using namespace Ttyh::Settings;
using namespace Ttyh::Versions;
using namespace Ttyh::Profiles;
using namespace Ttyh::Master;
using namespace Ttyh::News;
using namespace Ttyh::Storage;

template<typename T>
using QSP = QSharedPointer<T>;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationName("TtyhLauncher");
    QApplication::setApplicationVersion(PROJECT_VERSION);

    QTranslator t;
    QDate today = QDate::currentDate();
    if (today.month() == 4 && today.day() == 1) {
        t.load(":/translations/koi7.qm");
    } else {
        t.load(":/translations/ru.qm");
    }
    QApplication::installTranslator(&t);

    QCommandLineParser args;
    auto optHelp = args.addHelpOption();
    auto optVersion = args.addVersionOption();

    QCommandLineOption optDirectory({ "d", "directory" }, "Set the data directory", "dataDir");
    QCommandLineOption optStore({ "s", "store" }, "Set the store url", "storeUrl");
    QCommandLineOption optMaster({ "m", "master" }, "Set the master url", "masterUrl");

    args.addOption(optDirectory);
    args.addOption(optStore);
    args.addOption(optMaster);
    args.process(a);

    QString storeUrl = "https://store.ttyh.ru";
    QString masterUrl = "https://master.ttyh.ru";
    const QString newsUrl = "https://ttyh.ru/misc.php?page=feed";

    auto dataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString workDir = QString("%1/%2").arg(dataPath, "ttyhlauncher2");

    if (args.isSet(optHelp))
        args.showHelp();

    if (args.isSet(optVersion))
        args.showVersion();

    if (args.isSet(optStore))
        storeUrl = args.value(optStore);

    if (args.isSet(optMaster))
        masterUrl = args.value(optMaster);

    if (args.isSet(optDirectory)) {
        QDir dir(args.value(optDirectory));
        auto isAbsolute = dir.makeAbsolute();
        workDir = dir.absolutePath();

        if (!isAbsolute || !dir.exists()) {
            QTextStream(stderr) << QString("Invalid directory: '%1'").arg(workDir) << Qt::endl;
            return 1;
        }
    }

    const int logCount = 3;

    auto qDel = &QObject::deleteLater;
    auto nam = QSP<QNetworkAccessManager>(new QNetworkAccessManager(), qDel);
    auto logger = QSP<Logger>(new Logger(workDir, logCount), qDel);
    QObject::connect(logger.data(), &Logger::onLog,
                     [](const QString &line) { QTextStream(stdout) << line << Qt::endl; });

    auto settings = QSP<SettingsManager>(new SettingsManager(workDir, logger));
    if (settings->isFreshRun()) {
        Utils::Migrations::restoreLoginSettings(settings, logger);
    }

    auto profiles = QSP<ProfilesManager>(new ProfilesManager(workDir, logger));
    auto versions = QSP<VersionsManager>(new VersionsManager(workDir, storeUrl, nam, logger), qDel);

    auto ticket = settings->data.ticket;
    auto client = QSP<TtyhClient>(new TtyhClient(masterUrl, ticket, nam, logger), qDel);

    auto checker = QSP<FileChecker>(new FileChecker(workDir, logger), qDel);
    auto downloader = QSP<Downloader>(new Downloader(workDir, storeUrl, nam, logger), qDel);
    auto runner = QSP<ProfileRunner>(new ProfileRunner(workDir, logger), qDel);
    auto feed = QSP<NewsFeed>(new NewsFeed(newsUrl, nam, logger), qDel);

    Launcher l(settings, profiles, versions, client, checker, downloader, runner, feed, logger);
    l.start();

    return QApplication::exec();
}
