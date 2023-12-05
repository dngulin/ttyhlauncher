#include <QApplication>
#include <QtCore/QUuid>
#include <QtCore/QDir>
#include <QtCore/QHash>

#include "json/versionindex.h"
#include "utils/indexhelper.h"
#include "utils/platform.h"
#include "utils/zip.h"
#include "profilerunner.h"

using namespace Ttyh::Utils;

Ttyh::Profiles::ProfileRunner::ProfileRunner(QString dirName, const QSharedPointer<Logger> &logger)
    : dataPath(std::move(dirName)), log(logger, "Runner")
{
    connect(&game, &QProcess::started, [=]() {
        log.info("Started!");
        emit startHandled(true);
    });
    connect(&game, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
        switch (error) {
        case QProcess::FailedToStart:
            log.error("Failed to start!");
            break;
        case QProcess::Crashed:
            log.error("Crashed!");
            break;
        case QProcess::Timedout:
            log.error("Timeout!");
            break;
        case QProcess::ReadError:
            log.error("Read error!");
            break;
        case QProcess::WriteError:
            log.error("Write error!");
            break;
        case QProcess::UnknownError:
            log.error("Something went wrong!");
            break;
        }
        emit startHandled(false);
    });

    connect(&game, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                QString msg("Process finished with the exit code %1");
                log.info(msg.arg(QString::number(exitCode)));

                emit finished((exitStatus == QProcess::NormalExit) && (exitCode == 0));
            });

    connect(&game, &QProcess::readyReadStandardOutput,
            [=]() { logger->raw(game.readAllStandardOutput().trimmed()); });
    connect(&game, &QProcess::readyReadStandardError,
            [=]() { logger->raw(game.readAllStandardError().trimmed()); });
}

bool Ttyh::Profiles::ProfileRunner::run(const ProfileInfo &info, const QString &userName,
                                        const QSize &launcherSize)
{
    auto accessToken = QUuid::createUuid().toString().mid(1, 36);
    auto clientToken = QUuid::createUuid().toString().mid(1, 36);
    return run(info, userName, accessToken, clientToken, launcherSize);
}

bool Ttyh::Profiles::ProfileRunner::run(const ProfileInfo &info, const QString &userName,
                                        const QString &accessToken, const QString &clientToken,
                                        const QSize &launcherSize)
{
    auto name = info.name;
    auto data = info.data;

    auto version = data.version;
    log.info(QString("Starting the profile '%1' (%2)...").arg(name, version.toString()));

    auto versionIndexPath =
            QString("%1/versions/%2/%3.json").arg(dataPath, version.toString(), version.id);

    auto versionIndex = IndexHelper::load<Json::VersionIndex>(versionIndexPath);
    if (!versionIndex.isValid()) {
        log.error("Failed to load version index!");
        return false;
    }

    auto nativesPath = QString("%1/natives").arg(dataPath);
    QDir().mkpath(nativesPath);

    auto prefixLength = dataPath.length() + 1;
    QStringList classPath;
    foreach (auto lib, versionIndex.libraries) {
        if (!Platform::checkRules(lib.rules))
            continue;

        auto libPathInfo = Platform::getLibraryPathInfo(lib);
        auto libPath = QString("%1/libraries/%2").arg(dataPath, libPathInfo.path);

        if (!libPathInfo.isNativeLib) {
            classPath << libPath;
        } else {
            log.info(QString("Extracting '%1'...").arg(libPath.mid(prefixLength)));
            auto logError = [&](const QString &msg) { log.error(msg); };
            if (!Utils::Zip::unzipDir(libPath, nativesPath, logError)) {
                log.error(QString("Failed to extract '%1'!").arg(libPath.mid(prefixLength)));
                return false;
            }
        }
    }

    classPath << QString("%1/versions/%2/%3.jar").arg(dataPath, version.toString(), version.id);

    QStringList args;
    if (data.useCustomJavaArgs) {
        args << data.customJavaArgs.split(' ');
    }

    args << "-Dline.separator=\r\n";
    args << "-Dfile.encoding=UTF8";

    foreach (auto arg, getJvmArgs(versionIndex.javaArguments)) {
        arg.replace("${natives_directory}", nativesPath);
        arg.replace("${launcher_name}", QApplication::applicationName());
        arg.replace("${launcher_version}", QApplication::applicationVersion());
        arg.replace("${classpath}", classPath.join(Platform::getClassPathSeparator()));
        args << arg;
    }

    args << versionIndex.mainClass;

    auto profilePath = QString("%1/profiles/%2").arg(dataPath, name);
    QHash<QString, QString> argsMap;
    argsMap.insert("${version_name}", version.id);
    argsMap.insert("${version_type}", version.prefix);
    argsMap.insert("${auth_player_name}", userName);
    argsMap.insert("${auth_access_token}", accessToken);
    argsMap.insert("${auth_uuid}", clientToken);
    argsMap.insert("${assets_root}", QString("%1/assets").arg(dataPath));
    argsMap.insert("${assets_index_name}", versionIndex.assetsIndex);
    argsMap.insert("${game_directory}", profilePath);
    argsMap.insert("${user_properties}", "{}");
    argsMap.insert("${user_type}", "mojang");

    auto gameArgs = versionIndex.gameArguments;
    for (int i = 0; i < gameArgs.count(); i++) {
        auto token = gameArgs[i];

        if (argsMap.contains(token))
            gameArgs[i] = argsMap[token];
    }
    args << gameArgs;

    if (data.setWindowSizeOnRun) {
        auto mode = data.windowSizeMode;

        switch (mode) {
        case WindowSizeMode::FullScreen:
            args << "--fullscreen";
            break;

        case WindowSizeMode::LauncherLike:
        case WindowSizeMode::Specified:
            auto size = mode == WindowSizeMode::Specified ? data.windowSize : launcherSize;
            args << "--width" << QString::number(size.width());
            args << "--height" << QString::number(size.height());
            break;
        }
    }

    QDir().mkpath(profilePath);
    game.setWorkingDirectory(profilePath);

    auto javaPath = data.useCustomJavaPath ? data.customJavaPath : "java";

    log.info(javaPath + " " + args.join(' '));
    game.start(javaPath, args);
    return true;
}

QStringList Ttyh::Profiles::ProfileRunner::getJvmArgs(const QList<Json::ArgumentInfo> &args)
{
    if (args.isEmpty()) {
        QStringList fallback;
        fallback << "-Djava.library.path=${natives_directory}";
        fallback << "-Dminecraft.launcher.brand=${launcher_name}";
        fallback << "-Dminecraft.launcher.version=${launcher_version}";
        fallback << "-cp"
                 << "${classpath}";
        return fallback;
    }

    QStringList checkedArgs;
    foreach (auto arg, args) {
        if (Platform::checkRules(arg.rules)) {
            checkedArgs << arg.values;
        }
    }
    return checkedArgs;
}
