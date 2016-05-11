#include "gamerunner.h"

#include <QNetworkAccessManager>

#include "util.h"
#include "jsonparser.h"

GameRunner::GameRunner(const QString &login, const QString &pass,
                       bool onlineMode, const QRect &windowGeometry,
                       QObject *parent)
{
    setParent(parent);

    settings = Settings::instance();
    logger = Logger::logger();

    name = login;
    password = pass;

    isOnline = onlineMode;
    geometry = windowGeometry;

    version = settings->loadClientVersion();

    checker = new HashChecker();
    checker->moveToThread(&checkThread);

    connect(&checkThread, &QThread::finished, checker, &QObject::deleteLater);

    connect(this, &GameRunner::beginCheck, checker, &HashChecker::checkFiles);

    connect(checker, &HashChecker::verificationFailed,
            this, &GameRunner::onBadChecksumm);

    connect(checker, &HashChecker::finished,
            this, &GameRunner::runGame);

    checkThread.start();
}

GameRunner::~GameRunner()
{
    checkThread.quit();
    checkThread.wait();
}

void GameRunner::Run()
{
    log( tr("Prepare game for start...") );
    if (!isOnline)
    {
        log( tr("Offline mode selected.") );
    }

    requestAcessToken();
}

void GameRunner::requestAcessToken()
{
    if (isOnline)
    {
        log("Requesting acess token...");

        // Make JSON login request, see: http://wiki.vg/Authentication
        QJsonObject payload, agent, platform;

        agent["name"] = "Minecraft";
        agent["version"] = 1;

        platform["os"] = settings->getOsName();
        platform["version"] = settings->getOsVersion();
        platform["word"] = settings->getWordSize();

        QString ticket = settings->makeMinecraftUuid().remove('{').remove('}');

        payload["agent"] = agent;
        payload["platform"] = platform;
        payload["username"] = name;
        payload["password"] = password;
        payload["ticket"] = ticket;
        payload["launcherVersion"] = Settings::launcherVersion;

        QJsonDocument jsonRequest(payload);

        connect(&fetcher, &DataFetcher::finished,
                this, &GameRunner::acessTokenReceived);

        fetcher.makePost( Settings::authUrl, jsonRequest.toJson() );
    }
    else
    {
        log( tr("Generate acess token...") );

        QByteArray clientArray = QUuid::createUuid().toByteArray();
        QByteArray accessArray = QUuid::createUuid().toByteArray();

        clientToken = QString(clientArray).remove('{').remove('}');
        accessToken = QString(accessArray).remove('{').remove('}');

        determinateVersion();
    }
}

void GameRunner::acessTokenReceived(bool result)
{
    disconnect(&fetcher, &DataFetcher::finished,
               this, &GameRunner::acessTokenReceived);

    if (!result)
    {
        QString message = tr("Can't make login: ");
        emitError( message + fetcher.errorString() );
        return;
    }

    JsonParser parser;

    // Check for valid JSON reply
    if ( !parser.setJson( fetcher.getData() ) )
    {
        QString message = tr("Can't parse login reply: ");
        emitError( message + parser.getParserError() );
        return;
    }

    // Check for error response
    if ( parser.hasServerResponseError() )
    {
        QString message = tr("Error server answer: ");
        emitError( message + parser.getServerResponseError() );
        return;
    }

    // Get reply data
    clientToken = parser.getClientToken();
    accessToken = parser.getAccessToken();

    determinateVersion();
}

void GameRunner::determinateVersion()
{
    if (version == "latest")
    {
        log( tr("Try to determinate latest vesion...") );

        if (isOnline)
        {
            log( tr("Requesting version list...") );

            connect(&fetcher, &DataFetcher::finished,
                    this, &GameRunner::versionsListReceived);

            fetcher.makeGet( settings->getVersionsUrl() );
        }
        else
        {
            log("Looking for local versions...");

            JsonParser parser;

            QString time = "1991-05-18T13:15:00+07:00";
            QDateTime oldTime = QDateTime::fromString(time, Qt::ISODate);

            QString path = settings->getVersionsDir();
            foreach ( QString ver, QDir(path).entryList() )
            {
                QFile file(path + "/" + ver + "/" + ver + ".json");

                if ( file.open(QIODevice::ReadOnly) )
                {
                    if ( parser.setJson( file.readAll() ) )
                    {
                        if ( parser.hasReleaseTime() )
                        {
                            QDateTime relTime = parser.getReleaseTime();
                            if ( relTime.isValid() && (relTime > oldTime) )
                            {
                                oldTime = relTime;
                                version = ver;
                            }
                        }
                    }
                    file.close();
                }
            }

            if (version == "latest")
            {
                QString message = tr("Local versions not found");
                emitNeedUpdate(message);
                return;
            }
            else
            {
                log(tr("Latest local version: ") + version);
                updateVersionIndex();
            }
        }
    }
    else
    {
        updateVersionIndex();
    }
}

void GameRunner::versionsListReceived(bool result)
{
    disconnect(&fetcher, &DataFetcher::finished,
               this, &GameRunner::versionsListReceived);

    if (!result)
    {
        QString message = tr("Can't fetch latest version: ");
        emitError( message + fetcher.errorString() );
        return;
    }

    JsonParser parser;

    if ( !parser.setJson( fetcher.getData() ) )
    {
        QString message = tr("Can't parse latest version: ");
        emitError( message + parser.getParserError() );
        return;
    }

    if ( !parser.hasLatestReleaseVersion() )
    {
        emitError( tr("Latest version not defined at update server") );
        return;
    }

    version = parser.getLatestReleaseVersion();
    log(tr("Latest version: ") + version);

    updateVersionIndex();
}

void GameRunner::updateVersionIndex()
{
    if (isOnline)
    {
        log( tr("Updating version indexes...") );

        QString url = settings->getVersionUrl(version);
        QString dir = settings->getVersionsDir() + "/" + version + "/";

        QString versionIndexUrl = url + version + ".json";
        QString versionIndexPath = dir + version + ".json";

        QString dataIndexUrl = url + "data.json";
        QString dataIndexPath = dir + "data.json";

        downloader.reset();
        downloader.add(versionIndexUrl, versionIndexPath);
        downloader.add(dataIndexUrl, dataIndexPath);

        connect(&downloader, &FileFetcher::filesFetchFinished,
                this, &GameRunner::versionIndexesUpdated);

        downloader.fetchFiles();
    }
    else
    {
        checkFiles();
    }
}

void GameRunner::versionIndexesUpdated()
{
    disconnect(&downloader, &FileFetcher::filesFetchFinished,
               this, &GameRunner::versionIndexesUpdated);

    updateAssetsIndex();
}

void GameRunner::updateAssetsIndex()
{
    log( tr("Updating assets index...") );

    QString versionDir = settings->getVersionsDir() + "/" + version + "/";
    QString versionIndexPath = versionDir + version + ".json";

    JsonParser parser;

    if ( parser.setJsonFromFile(versionIndexPath) )
    {
        if ( parser.hasAssetsVersion() )
        {
            QString assets = parser.getAssetsVesrsion();

            QString url = settings->getAssetsUrl();
            QString dir = settings->getAssetsDir();

            QString assetsUrl = url + "indexes/" + assets + ".json";
            QString assetsPath = dir + "/indexes/" + assets + ".json";

            downloader.reset();
            downloader.add(assetsUrl, assetsPath);

            connect(&downloader, &FileFetcher::filesFetchFinished,
                    this, &GameRunner::assetsIndexUpdated);

            downloader.fetchFiles();
        }
        else
        {
            emitError( tr("Version index not conatins assets version.") );
            return;
        }
    }
    else
    {
        QString message = tr("Can't parse version index: ");
        emitError( message + parser.getParserError() );
        return;
    }
}

void GameRunner::assetsIndexUpdated()
{
    disconnect(&downloader, &FileFetcher::filesFetchFinished,
               this, &GameRunner::assetsIndexUpdated);

    checkFiles();
}

void GameRunner::checkFiles()
{
    log( tr("Prepare file list for checking...") );

    JsonParser dataParser, assetsParser;

    QString versionDir = settings->getVersionsDir() + "/" + version + "/";
    QString versionIndexPath = versionDir + version + ".json";
    QString dataIndexPath = versionDir + "data.json";

    if ( !versionParser.setJsonFromFile(versionIndexPath) )
    {
        QString message = tr("Can't parse version index: ");
        emitError( message + versionParser.getParserError() );
        return;
    }

    if ( !dataParser.setJsonFromFile(dataIndexPath) )
    {
        QString message = tr("Can't parse data index: ");
        emitError( message + dataParser.getParserError() );
        return;
    }

    // Main JAR
    if ( !dataParser.hasJarFileInfo() )
    {
        emitError( tr("Main JAR does not described in data index.") );
        return;
    }

    FileInfo jar;

    jar.name = versionDir + version + ".jar";
    jar.hash = dataParser.getJarFileInfo().hash;
    jar.size = dataParser.getJarFileInfo().size;
    jar.url = settings->getVersionUrl(version) + version + ".jar";

    checkList.append(jar);

    // Libraries
    if ( !versionParser.hasLibraryList() )
    {
        emitError( tr("Libraries are not described in data index.") );
        return;
    }

    QString libDir = settings->getLibsDir() + "/";
    QString libUrl = settings->getLibsUrl();

    QList<LibraryInfo> liblist = versionParser.getLibraryList();
    foreach (LibraryInfo entry, liblist)
    {
        QString lib = entry.name;

        if ( !dataParser.hasLibFileInfo(lib) )
        {
            emitError(tr("Data index does not contain library: ") + lib);
            return;
        }

        FileInfo fileInfo = dataParser.getLibFileInfo(lib);

        fileInfo.name = libDir + lib;
        fileInfo.url = libUrl + lib;

        checkList.append(fileInfo);
    }

    // Addons
    if ( !dataParser.hasAddonsFilesInfo() )
    {
        emitError( tr("Addons are not described in data index.") );
        return;
    }

    QString clientPrefix = settings->getClientPrefix(version) + "/";
    QString addonsUrl = settings->getVersionUrl(version) + "files/";

    QList<FileInfo> addons = dataParser.getAddonsFilesInfo();
    foreach (FileInfo addon, addons)
    {
        QString shortName = addon.name;
        addon.name = clientPrefix + shortName;
        addon.url = addonsUrl + shortName;

        checkList.append(addon);
    }

    // Assets
    if ( settings->loadClientCheckAssetsState() )
    {
        if ( !versionParser.hasAssetsVersion() )
        {
            emitError( tr("Assets are not described in version index.") );
            return;
        }

        QString assetsIndexDir = settings->getAssetsDir() + "/indexes/";
        QString assetsName = versionParser.getAssetsVesrsion() + ".json";

        if ( !assetsParser.setJsonFromFile(assetsIndexDir + assetsName) )
        {
            QString message = tr("Can't parse assets index: ");
            emitError( message + assetsParser.getParserError() );
            return;
        }

        if ( !assetsParser.hasAssetsList() )
        {
            emitError( tr("Assets list are not described in index.") );
            return;
        }

        QString assetsDir = settings->getAssetsDir() + "/objects/";
        QString assetsUrl = settings->getAssetsUrl() + "objects/";

        QList<FileInfo> assets = assetsParser.getAssetsList();
        foreach (FileInfo asset, assets)
        {
            QString shortName = asset.name;
            asset.name = assetsDir + shortName;
            asset.url = assetsUrl + shortName;

            checkList.append(asset);
        }
    }

    log( tr("Begin files check...") );
    emit beginCheck(checkList);
}

void GameRunner::onBadChecksumm(const FileInfo fileInfo)
{
    Q_UNUSED(fileInfo);

    checker->cancel();

    emitNeedUpdate( tr("Bad checksumms.") );
}

void GameRunner::runGame()
{
    log( tr("Prepare run data...") );

    // Run game with known uuid, acess token and game version
    QString java, nativesPath, classpath, cpSep,
            assetsName, mainClass, minecraftArguments;

    bool loadCustomJava = settings->loadClientJavaState();
    java = loadCustomJava ? settings->loadClientJava() : "java";

    bool isWindows = settings->getOsName() == "windows";
    cpSep = isWindows ? ";" : ":";

    nativesPath = settings->getNativesDir();
    Util::removeAll(nativesPath);

    QDir libsDir = QDir(nativesPath);
    libsDir.mkpath(nativesPath);

    if ( !libsDir.exists() )
    {
        emitError( tr("Can't create natives directory") );
        return;
    }

    QString libDir = settings->getLibsDir() + "/";

    QList<LibraryInfo> libList = versionParser.getLibraryList();
    foreach (LibraryInfo libInfo, libList)
    {
        if (libInfo.isNative)
        {
            Util::unzipArchive(libDir + libInfo.name, nativesPath);
        }
        else
        {
            classpath += libDir + libInfo.name + cpSep;
        }
    }

    QString versionsDir = settings->getVersionsDir() + "/";
    classpath += versionsDir + "/" + version + "/" + version + ".jar";

    if ( !versionParser.hasMainClass() )
    {
        QString message = tr("No main class in ") + version + ".json: ";
        emitError(message);
        return;
    }
    mainClass = versionParser.getMainClass();

    if ( !versionParser.hasAssetsVersion() )
    {
        QString message = tr("No assets in ") + version + ".json: ";
        emitError(message);
        return;
    }
    assetsName = versionParser.getAssetsVesrsion();

    if ( !versionParser.hasMinecraftArgs() )
    {
        QString message = tr("No minecraft args in ") + version + ".json: ";
        emitError(message);
        return;
    }
    minecraftArguments = versionParser.getMinecraftArgs();

    QString clientPrefix = settings->getClientPrefix(version);
    QString assetsDir = settings->getAssetsDir();
    QStringList mcArgList;
    foreach ( QString mcArg, minecraftArguments.split(" ") )
    {
        mcArg.replace("${auth_player_name}", name);
        mcArg.replace("${version_name}", version);
        mcArg.replace("${game_directory}", clientPrefix);
        mcArg.replace("${assets_root}", assetsDir);
        mcArg.replace("${assets_index_name}", assetsName);
        mcArg.replace("${auth_uuid}", clientToken);
        mcArg.replace("${auth_access_token}", accessToken);
        mcArg.replace("${user_properties}", "{}");
        mcArg.replace("${user_type}", "mojang");

        int active = settings->loadActiveClientID();
        QString clientType = settings->getClientName( active );
        mcArg.replace("${version_type}", clientType);

        mcArgList << mcArg;
    }

    // Width & height/fullscreen args
    if ( settings->loadClientWindowSizeState() )
    {
        if ( settings->loadClientFullscreenState() )
        {
            mcArgList << "--fullscreen";
        }
        else
        {
            QRect mcRect;
            if ( settings->loadClientUseLauncherSizeState() )
            {
                mcRect = geometry;
            }
            else
            {
                mcRect = settings->loadClientWindowGeometry();
            }

            mcArgList << "--width" << QString::number( mcRect.width() );
            mcArgList << "--height" << QString::number( mcRect.height() );
        }
    }

    // RUN RUN RUN
    QStringList argList;

    // Workaround for Oracle Java + StartSSL
    QString keystore = settings->getConfigDir() + "/keystore.ks";
    QString newline = "\r\n";

    argList << "-Djavax.net.ssl.trustStore=" + keystore
            << "-Djavax.net.ssl.trustStorePassword=123456"; // LOL
    argList << "-Dline.separator=" + newline;
    argList << "-Dfile.encoding=UTF8";

    // Setup user args
    QStringList userArgList;
    if ( settings->loadClientJavaArgsState() )
    {
        userArgList = settings->loadClientJavaArgs().split(" ");
    }

    if ( !userArgList.isEmpty() )
    {
        argList << userArgList;
    }

    argList << "-Djava.library.path=" + nativesPath
            << "-cp" << classpath
            << mainClass
            << mcArgList;

    minecraft.setProcessChannelMode(QProcess::MergedChannels);

    QDir(clientPrefix).mkpath(clientPrefix);
    minecraft.setWorkingDirectory(clientPrefix);

    connect(&minecraft, &QProcess::readyReadStandardOutput,
            this, &GameRunner::gameLog);

    connect(&minecraft, &QProcess::readyReadStandardError,
            this, &GameRunner::gameLog);

    connect( &minecraft, SIGNAL( error(QProcess::ProcessError) ),
             this, SLOT( onGameError(QProcess::ProcessError) ) );

    connect( &minecraft, SIGNAL( finished(int) ), this,
             SLOT( onGameFinished(int) ) );

    log( tr("Run string: ") + java + " " + argList.join(' ') );
    minecraft.start(java, argList);
}

void GameRunner::log(const QString &text)
{
    logger->appendLine(tr("GameRunner"), text);
}

void GameRunner::gameLog()
{
    logger->appendLine( tr("Game"), minecraft.readAll().trimmed() );
}

void GameRunner::onGameError(QProcess::ProcessError error)
{
    switch (error)
    {
    case QProcess::FailedToStart:
        emitError( tr("Failed to start: ") + minecraft.errorString() );
        break;

    case QProcess::Crashed:
        emitError( tr("Game crashed: ") + minecraft.errorString() );
        break;

    case QProcess::Timedout:
        emitError( tr("Game start timeout: ") + minecraft.errorString() );
        break;

    case QProcess::WriteError:
        emitError( tr("Game can't write: ") + minecraft.errorString() );
        break;

    case QProcess::ReadError:
        emitError( tr("Game can't read: ") + minecraft.errorString() );
        break;

    case QProcess::UnknownError:
    default:
        emitError( tr("Mysterious error: ") + minecraft.errorString() );
        break;
    }
}

void GameRunner::onGameStarted()
{
    log( tr("Game started") );
    emit started();
}

void GameRunner::onGameFinished(int exitCode)
{
    log( tr("Game finished with code ") + QString::number(exitCode) );
    emit finished(exitCode);
}

void GameRunner::emitError(const QString &message)
{
    log(tr("Error: ") + message);
    emit error(message);
}

void GameRunner::emitNeedUpdate(const QString &message)
{
    log(tr("Need update: ") + message);
    emit needUpdate(message);
}
