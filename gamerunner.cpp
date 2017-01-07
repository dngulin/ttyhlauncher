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
            this, &GameRunner::onBadChecksum);

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
        log( tr("Requesting access token...") );

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
        QString message = tr("Can't make login! %1");
        emitError( message.arg( fetcher.errorString() ) );
        return;
    }

    JsonParser parser;

    // Check for valid JSON reply
    if ( !parser.setJson( fetcher.getData() ) )
    {
        QString message = tr("Can't parse login reply! %1");
        emitError( message.arg( parser.getParserError() ) );
        return;
    }

    // Check for error response
    if ( parser.hasServerResponseError() )
    {
        QString message = tr("Bad server answer! %1");
        emitError( message.arg( parser.getServerResponseError() ) );
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
                QString message = tr("Local versions not found.");
                emitError(message);
                return;
            }
            else
            {
                log( tr("Latest local version: %1").arg(version) );
                checkIndexes();
            }
        }
    }
    else
    {
        checkIndexes();
    }
}

void GameRunner::versionsListReceived(bool result)
{
    disconnect(&fetcher, &DataFetcher::finished,
               this, &GameRunner::versionsListReceived);

    if (!result)
    {
        QString message = tr("Can't fetch latest version! %1");
        emitError( message.arg( fetcher.errorString() ) );
        return;
    }

    JsonParser parser;

    if ( !parser.setJson( fetcher.getData() ) )
    {
        QString message = tr("Can't parse latest version! %1");
        emitError( message.arg( parser.getParserError() ) );
        return;
    }

    if ( !parser.hasLatestReleaseVersion() )
    {
        emitError( tr("Latest version not defined at update server!") );
        return;
    }

    version = parser.getLatestReleaseVersion();
    log( tr("Latest version: %1").arg(version) );

    checkIndexes();
}

void GameRunner::checkIndexes()
{
    if (isOnline)
    {
        log( tr("Checking indexes...") );

        QString verDir = settings->getVersionsDir() + "/" + version + "/";
        versionIndexPath = verDir + version + ".json";
        dataIndexPath = verDir + "data.json";

        QString verUrl = settings->getVersionUrl(version);
        versionIndexUrl = verUrl + version + ".json";
        dataIndexUrl = verUrl + "data.json";

        // First request in chain: versionIndex, dataIndex, assetsIndex
        requestVersionIndex();
    }
    else
    {
        checkFiles();
    }
}

void GameRunner::requestVersionIndex()
{
    if ( QFile::exists(versionIndexPath) )
    {
        log( tr("Requesting version index...") );
        connect(&fetcher, &DataFetcher::finished,
                this, &GameRunner::versionIndexReceived);
        fetcher.makeGet(versionIndexUrl);
    }
    else
    {
        emitNeedUpdate( tr("Need to download running version!") );
        return;
    }
}

void GameRunner::versionIndexReceived(bool result)
{
    disconnect(&fetcher, &DataFetcher::finished,
               this, &GameRunner::versionIndexReceived);

    if (result)
    {
        QString hash = HashChecker::getDataHash( fetcher.getData() );
        if ( HashChecker::isFileHashValid(versionIndexPath, hash) )
        {
            requestDataIndex();
        }
        else
        {
            emitNeedUpdate( tr("Version index is obsolete!") );
            return;
        }
    }
    else
    {
        requestDataIndex();
    }
}

void GameRunner::requestDataIndex()
{
    if ( QFile::exists(dataIndexPath) )
    {
        log( tr("Requesting data index...") );
        connect(&fetcher, &DataFetcher::finished,
                this, &GameRunner::dataIndexReceived);
        fetcher.makeGet(dataIndexUrl);
    }
    else
    {
        emitNeedUpdate( tr("Need to download running version!") );
        return;
    }
}

void GameRunner::dataIndexReceived(bool result)
{
    disconnect(&fetcher, &DataFetcher::finished,
               this, &GameRunner::dataIndexReceived);

    if (result)
    {
        QString hash = HashChecker::getDataHash( fetcher.getData() );
        if ( HashChecker::isFileHashValid(dataIndexPath, hash) )
        {
            requestAssetsIndex();
        }
        else
        {
            emitNeedUpdate( tr("Data index is obsolete!") );
        }
    }
    else
    {
        requestAssetsIndex();
    }
}

void GameRunner::requestAssetsIndex()
{
    JsonParser parser;
    if ( parser.setJsonFromFile(versionIndexPath) && parser.hasAssetsVersion() )
    {
        QString assets = parser.getAssetsVesrsion();

        QString dir = settings->getAssetsDir();
        QString url = settings->getAssetsUrl();

        assetsIndexPath = dir + "/indexes/" + assets + ".json";
        assetsIndexUrl = url + "indexes/" + assets + ".json";

        if ( QFile::exists(assetsIndexPath) )
        {
            log( tr("Requesting assets index...") );
            connect(&fetcher, &DataFetcher::finished,
                    this, &GameRunner::assetsIndexReceived);
            fetcher.makeGet(assetsIndexUrl);
        }
        else
        {
            emitNeedUpdate( tr("Need to download running version!") );
            return;
        }
    }
    else
    {
        checkFiles();
    }
}

void GameRunner::assetsIndexReceived(bool result)
{
    disconnect(&fetcher, &DataFetcher::finished,
               this, &GameRunner::assetsIndexReceived);

    if (result)
    {
        QString hash = HashChecker::getDataHash( fetcher.getData() );
        if ( HashChecker::isFileHashValid(assetsIndexPath, hash) )
        {
            checkFiles();
        }
        else
        {
            emitNeedUpdate( tr("Assets index is obsolete!") );
            return;
        }
    }
    else
    {
        checkFiles();
    }
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
        QString message = tr("Can't parse version index! %1");
        emitError( message.arg( versionParser.getParserError() ) );
        return;
    }

    if ( !dataParser.setJsonFromFile(dataIndexPath) )
    {
        QString message = tr("Can't parse data index! %1");
        emitError( message.arg( dataParser.getParserError() ) );
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
            emitError( tr("Data index not contains library: %1").arg(lib) );
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
    bool stopOnBadHash = isOnline;
    emit beginCheck(checkList, stopOnBadHash);
}

void GameRunner::onBadChecksum(const FileInfo fileInfo)
{
    log( tr("Bad checksum for %1.").arg(fileInfo.name) );
    if (isOnline)
    {
        emitNeedUpdate( tr("Client files are obsolete.") );
    }
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
        emitError( tr("Can't create natives directory!") );
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
        QString message = tr("No main class in %1.json!").arg(version);
        emitError(message);
        return;
    }
    mainClass = versionParser.getMainClass();

    if ( !versionParser.hasAssetsVersion() )
    {
        QString message = tr("No assets in %1.json!").arg(version);
        emitError(message);
        return;
    }
    assetsName = versionParser.getAssetsVesrsion();

    if ( !versionParser.hasMinecraftArgs() )
    {
        QString message = tr("No minecraft args in %1.json!").arg(version);
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
        QString clientType = settings->getClientName(active);
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
            << "-Djavax.net.ssl.trustStorePassword=123456"; // SO SAFE
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

    connect(&minecraft, &QProcess::started,
            this, &GameRunner::onGameStarted);

    connect(&minecraft, &QProcess::readyReadStandardOutput,
            this, &GameRunner::gameLog);

    connect(&minecraft, &QProcess::readyReadStandardError,
            this, &GameRunner::gameLog);

    connect( &minecraft, SIGNAL(error(QProcess::ProcessError)),
             this, SLOT(onGameError(QProcess::ProcessError)));

    connect( &minecraft, SIGNAL(finished(int)), this,
             SLOT(onGameFinished(int)));

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
    QString errstr = minecraft.errorString();

    switch (error)
    {
    case QProcess::FailedToStart:
        emitError( tr("Failed to start! Java not found!") );
        break;

    case QProcess::Crashed:
        emitError( tr("Game crashed! %1").arg(errstr) );
        break;

    case QProcess::Timedout:
        emitError( tr("Game start timeout! %1").arg(errstr) );
        break;

    case QProcess::WriteError:
        emitError( tr("Game can't write! %1").arg(errstr) );
        break;

    case QProcess::ReadError:
        emitError( tr("Game can't read! %1").arg(errstr) );
        break;

    case QProcess::UnknownError:
    default:
        emitError( tr("Mysterious error! %1").arg(errstr) );
        break;
    }
}

void GameRunner::onGameStarted()
{
    log( tr("Game started.") );
    emit started();
}

void GameRunner::onGameFinished(int exitCode)
{
    log( tr("Game finished with code %1.").arg(exitCode) );
    emit finished(exitCode);
}

void GameRunner::emitError(const QString &message)
{
    log( tr("Error! %1").arg(message) );
    emit error(message);
}

void GameRunner::emitNeedUpdate(const QString &message)
{
    log( tr("Need update! %1").arg(message) );
    emit needUpdate(message);
}
