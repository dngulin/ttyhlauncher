#include "gamerunner.h"

#include <QNetworkAccessManager>

#include "util.h"
#include "jsonparser.h"

GameRunner::GameRunner(const QString &playerLogin,
                       const QString &playerPassword,
                       const QString &gamePrefix,
                       bool onlineMode, QObject *parent)
{
    setParent(parent);

    settings = Settings::instance();
    logger = Logger::logger();

    name = playerLogin;
    password = playerPassword;
    prefix = gamePrefix;
    isOnline = onlineMode;
}

void GameRunner::run()
{
    logger->append("GameRunner", "Prepare game running...\n");
    QNetworkAccessManager* nam = new QNetworkAccessManager();

    // Prepare run data
    QString uuid;
    QString accessToken;
    QString gameVersion;

    JsonParser jsonParser;

    // Online mode
    if (isOnline)
    {
        // Make JSON login request, see: http://wiki.vg/Authentication
        QJsonObject payload, agent, platform;

        agent["name"] = "Minecraft";
        agent["version"] = 1;

        platform["os"] = settings->getOsName();
        platform["version"] = settings->getOsVersion();
        platform["word"] = settings->getWordSize();

        payload["agent"] = agent;
        payload["platform"] = platform;
        payload["username"] = name;
        payload["password"] = password;
        payload["ticket"] =
                settings->makeMinecraftUuid().remove('{').remove('}');
        payload["launcherVersion"] = Settings::launcherVersion;

        QJsonDocument jsonRequest(payload);

        logger->append("GameRunner", "Making login request...\n");
        Reply loginReply =
                Util::makePost(nam, Settings::authUrl, jsonRequest.toJson());

        // Check for success reply
        if (!loginReply.isSuccess())
        {
            QString message = "Can't make login: ";
            emitError(message + loginReply.getErrorString());
            return;
        }

        // Check for valid JSON reply
        if ( !jsonParser.setJson(loginReply.getData()) )
        {
            QString message = "Can't parse login reply: ";
            emitError(message + jsonParser.getParserError());
            return;
        }

        // Check for error response
        if (jsonParser.hasServerResponseError())
        {
            QString message = "Server response: ";
            emitError(message + jsonParser.getServerResponseError());
            return;
        }

        // Get reply data
        uuid = jsonParser.getClientToken();
        accessToken = jsonParser.getAccessToken();
        gameVersion = settings->loadClientVersion();

        // Get latest version from server
        if (gameVersion == "latest")
        {
            Reply versionReply = Util::makeGet(nam, settings->getVersionsUrl());

            // Chect for success reply
            if (!versionReply.isSuccess())
            {
                QString message = "Can't parse latest version: ";
                emitError(message + jsonParser.getParserError());
                return;
            }

            // Check for valid JSON
            if ( !jsonParser.setJson(versionReply.getData()) )
            {
                QString message = "Can't parse latest version: ";
                emitError(message + versionReply.getErrorString());
                return;
            }

            // Check for latest version in response
            if (!jsonParser.hasLatestReleaseVersion())
            {
                QString message = "No latest version info in server response";
                emitError(message);
                return;
            }

            gameVersion = jsonParser.getLatestReleaseVersion();
        }

        // Update json indexes before run (version, data, assets)
        logger->append(this->objectName(),
                       "Updating game indexes..." + gameVersion + "\n");

        QString versionUrl = settings->getVersionUrl(gameVersion);
        QString versionDir =
                settings->getVersionsDir() + "/" + gameVersion + "/";

        // Update version JSON
        QString verJsonUrl  = versionUrl + gameVersion + ".json";
        QString verJsonPath = versionDir + gameVersion + ".json";
        Util::downloadFile(nam, verJsonUrl, verJsonPath);

        // Update data JSON
        QString dataJsonUrl  = versionUrl + "data.json";
        QString dataJsonPath = versionDir + "data.json";
        Util::downloadFile(nam, dataJsonUrl, dataJsonPath);

        // Update assets JSON
        if (jsonParser.setJsonFromFile(verJsonPath))
        {
            if (jsonParser.hasAssets())
            {
                QString assets = jsonParser.getAssets();
                QString assetsUrl  = settings->getAssetsUrl()
                        + "indexes/" + assets + ".json";
                QString assetsPath = settings->getAssetsDir()
                        + "/indexes/" + assets + ".json";

                Util::downloadFile(nam, assetsUrl, assetsPath);
            }
        }

    }
    // Offline mode
    else
    {
        QByteArray uuidArray = QUuid::createUuid().toByteArray();
        uuid = QString(uuidArray).remove('{').remove('}');

        QByteArray atArray = QUuid::createUuid().toByteArray();
        accessToken = QString(atArray).remove('{').remove('}');

        gameVersion = settings->loadClientVersion();

        if (gameVersion == "latest") {
            // Great old date for comparsion
            QDateTime oldTime =
                    QDateTime::fromString("1991-05-18T13:15:00+07:00",
                                          Qt::ISODate);

            QDir versionDir = QDir(settings->getVersionsDir());
            QStringList versionList = versionDir.entryList();

            foreach (QString currentVersion, versionList)
            {
                QFile versionFile(settings->getVersionsDir()
                                  + "/" + currentVersion
                                  + "/" + currentVersion + ".json");

                if (versionFile.open(QIODevice::ReadOnly))
                {
                    if (jsonParser.setJson(versionFile.readAll()))
                        if (jsonParser.hasReleaseTime())
                        {
                            QDateTime relTime = jsonParser.getReleaseTime();
                            if (relTime.isValid() && (relTime > oldTime))
                            {
                                oldTime = relTime;
                                gameVersion = currentVersion;
                            }
                        }
                    versionFile.close();
                }
            }

            if (gameVersion == "latest")
            {
                QString message = "Not found any local version";
                emitNeedUpdate(message);
                return;
            }
        }
    }

    // Run game with known uuid, acess token and game version
    QString java, nativesPath, classpath, cpSep,
            assetsName, mainClass, minecraftArguments;

    if (settings->loadClientJavaState())
        java = settings->loadClientJava();
    else
        java = "java";

    if (settings->getOsName() == "windows") cpSep= ";";
    else cpSep= ":";

    nativesPath = settings->getNativesDir();
    Util::removeAll(nativesPath);

    QDir libsDir = QDir(nativesPath);
    libsDir.mkpath(nativesPath);

    if (!libsDir.exists())
    {
        QString message = "Can't create natives directory";
        emitError(message);
        return;
    }

    JsonParser versionParser;

    QString versionJsonName = settings->getVersionsDir()
            + "/" + gameVersion
            + "/" + gameVersion + ".json";
    if (!versionParser.setJsonFromFile(versionJsonName))
    {
        QString message = "Can't parse " + gameVersion + ".json: ";
        emitError(message + versionParser.getParserError());
        return;
    }

    //FIXME: create recursive function for inherits version indexes
    // Process library list
    if (!versionParser.hasLibraryList())
    {
        QString message = "No library list in " + gameVersion + ".json: ";
        emitError(message);
        return;
    }

    QList<LibraryInfo> libList = versionParser.getLibraryList();
    foreach (LibraryInfo libInfo, libList)
    {
        if (libInfo.isNative)
        {
            Util::unzipArchive(settings->getLibsDir() + "/" + libInfo.name,
                               nativesPath);
        }
        else
        {
            classpath += settings->getLibsDir() + "/" + libInfo.name + cpSep;
        }
    }

    // Add game jar to classpath
    // FIXME: need to read mainJar
    classpath += settings->getVersionsDir() + "/"
            + gameVersion + "/" + gameVersion + ".jar";

    // Check for mainClass
    if (!versionParser.hasMainClass())
    {
        QString message = "No main class in " + gameVersion + ".json: ";
        emitError(message);
        return;
    }
    mainClass = versionParser.getMainClass();

    // Check for assets
    if (!versionParser.hasAssets())
    {
        QString message = "No assets in " + gameVersion + ".json: ";
        emitError(message);
        return;
    }
    assetsName = versionParser.getAssets();

    // FIXME: implement checker: mainClass, libs, assets, addons

    // Check for minecraft args
    if (!versionParser.hasMinecraftArgs())
    {
        QString message = "No minecraft args in " + gameVersion + ".json: ";
        emitError(message);
        return;
    }
    minecraftArguments = versionParser.getMinecraftArgs();

    QStringList mcArgList;
    foreach (QString mcArg, minecraftArguments.split(" "))
    {
        mcArg.replace("${auth_player_name}",  name);
        mcArg.replace("${version_name}",      gameVersion);
        mcArg.replace("${game_directory}",    settings->getClientPrefix(gameVersion));
        mcArg.replace("${assets_root}",       settings->getAssetsDir());
        mcArg.replace("${assets_index_name}", assetsName);
        mcArg.replace("${auth_uuid}",         uuid);
        mcArg.replace("${auth_access_token}", accessToken);
        mcArg.replace("${user_properties}",   "{}");
        mcArg.replace("${user_type}",         "mojang");

        mcArgList << mcArg;
    }

    // Width & height/fullscreen args
    if(settings->loadClientWindowSizeState())
    {
        if(settings->loadClientFullscreenState())
        {
            mcArgList << "--fullscreen";
        }
        else
        {
            QRect mcRect;
            if(settings->loadClientUseLauncherSizeState())
            {
                //FIXME
                mcRect.setWidth(800);
                mcRect.setHeight(600);
            }
            else mcRect = settings->loadClientWindowGeometry();

            mcArgList << "--width"  << QString::number(mcRect.width());
            mcArgList << "--height" << QString::number(mcRect.height());
        }
    }

    // RUN RUN RUN
    QStringList argList;

    // Workaround for Oracle Java + StartSSL
    argList << "-Djavax.net.ssl.trustStore=" + settings->getConfigDir()
               + "/keystore.ks"
            << "-Djavax.net.ssl.trustStorePassword=123456";
    argList << "-Dline.separator=\r\n";
    argList << "-Dfile.encoding=UTF8";

    // Setup user args
    QStringList userArgList;
    if (settings->loadClientJavaArgsState())
        userArgList = settings->loadClientJavaArgs().split(" ");

    if (!userArgList.isEmpty()) argList << userArgList;

    argList << "-Djava.library.path=" + nativesPath
            << "-cp" << classpath
            << mainClass
            << mcArgList;

    QProcess minecraft;
    minecraft.setProcessChannelMode(QProcess::MergedChannels);

    // Set working directory
    QDir(settings->getClientPrefix(gameVersion))
            .mkpath(settings->getClientPrefix(gameVersion));
    minecraft.setWorkingDirectory(settings->getClientPrefix(gameVersion));

    logger->append("GameRunner",
                   "Run string: " + java + " " + argList.join(' ') + "\n");

    logger->append("GameRunner", "Try to start game...\n");
    minecraft.start(java, argList);
    if (!minecraft.waitForStarted())
    {
        QString message = "Can't run game: ";
        emitError(message + minecraft.errorString());
        return;
    }

    logger->append("GameRunner", "Process started\n");
    emit started();

    while (minecraft.state() == QProcess::Running)
        if (minecraft.waitForReadyRead())
            logger->append("Client", minecraft.readAll());

    logger->append("GameRunner", "Process finished with exit code "
                   + QString::number(minecraft.exitCode()) + "\n");
    emit finished(minecraft.exitCode());

    delete nam;
}

void GameRunner::emitError(const QString &message)
{
    QString pre = "Error: ";
    logger->append("GameRunner", pre + message + "\n");
    emit error(message);
}

void GameRunner::emitNeedUpdate(const QString &message)
{
    QString pre = "Need update: ";
    logger->append("GameRunner", pre + message + "\n");
    emit needUpdate(message);
}
