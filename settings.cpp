#include <QtCore>
#include <QStandardPaths>
#include <QSysInfo>

#include "settings.h"
#include "filefetcher.h"
#include "jsonparser.h"

#include "util.h"

typedef QStandardPaths Path;

const QString Settings::launcherVersion = "1.2.3";

const QString Settings::newsFeed = "https://ttyh.ru/misc.php?page=feed";

const QString Settings::updateServer = "http://store.ttyh.ru";
const QString Settings::buildServer = "https://ttyh.ru/builds";

// Master-server links
const QString Settings::master = "https://master.ttyh.ru/index.php";
const QString Settings::authUrl = master + "?act=login";
const QString Settings::changePasswrdUrl = master + "?act=chpass";
const QString Settings::skinUploadUrl = master + "?act=setskin";
const QString Settings::feedbackUrl = master + "?act=feedback";

const int Settings::timeout = 3000;

Settings *Settings::myInstance = NULL;
Settings *Settings::instance()
{
    if (myInstance == NULL)
    {
        myInstance = new Settings();
    }
    return myInstance;
}

Settings::Settings() : QObject()
{
    latestVersion = launcherVersion;

    nam = new QNetworkAccessManager(this);

    Path::StandardLocation dataLocation = Path::GenericDataLocation;
    Path::StandardLocation configLocation = Path::GenericConfigLocation;

    dataPath = Path::writableLocation(dataLocation) + "/ttyh_minecraft";
    configPath = Path::writableLocation(configLocation) + "/ttyhlauncher";

    // Prepare data and config directories
    QDir(dataPath).mkpath(dataPath);
    QDir(configPath).mkpath(configPath);

    settings = new QSettings(configPath + "/config.ini", QSettings::IniFormat);
}

void Settings::log(const QString &text)
{
    Logger::logger()->appendLine(tr("Settings"), text);
}

void Settings::updateLocalData()
{
    QUrl keystoreUrl(updateServer + "/store.ks");
    QString keystorePath = configPath + "/keystore.ks";

    QUrl clientsUrl(updateServer + "/prefixes.json");
    QString clientsPath = dataPath + "/prefixes.json";

    log( tr("Updating local data...") );

    FileFetcher fetcher;
    fetcher.setHiddenLenght(0);
    fetcher.add(keystoreUrl, keystorePath);
    fetcher.add(clientsUrl, clientsPath);

    QEventLoop loop;
    QObject::connect(&fetcher, &FileFetcher::filesFetchFinished,
                     &loop, &QEventLoop::quit);

    fetcher.fetchFiles();
    loop.exec();

    JsonParser parser;
    if ( parser.setJsonFromFile(clientsPath) )
    {
        if ( parser.hasPrefixesList() )
        {
            clients = parser.getPrefixesList();
        }
        else
        {
            log( tr("Error: no prefixes in prefixes.json") );
        }
    }
    else
    {
        log( tr("Error! %1").arg( parser.getParserError() ) );
    }
}

void Settings::fetchLatestVersion()
{
    QString arch = getWordSize();
    QUrl versionUrl(buildServer + "/build-" + arch + "-latest/version.txt");
    DataFetcher fetcher;

    QEventLoop loop;

    bool fetched = false;

    QObject::connect(&fetcher, &DataFetcher::finished,
                     [&loop, &fetched](bool result)
    {
        fetched = result;
        loop.quit();
    });

    log( tr("Requesting latest launcher version...") );
    fetcher.makeGet(versionUrl);
    loop.exec();

    if (fetched)
    {
        latestVersion = QString( fetcher.getData() ).trimmed();
        log( tr("Latest launcher version: %1.").arg(latestVersion) );
    }
    else
    {
        log( tr("Error! Latest launcher version not received!") );
    }
}

QString Settings::getlatestVersion() const
{
    return latestVersion;
}

QString Settings::getVersionsUrl() const
{
    QString client = getClientName( loadActiveClientID() );
    return updateServer + "/" + client + "/versions/versions.json";
}

QString Settings::getVanillaVersionsUrl() const
{
    return QString(
        "http://s3.amazonaws.com/Minecraft.Download/versions/versions.json");
}

QString Settings::getVersionUrl(const QString &version)
{
    QString client = getClientName( loadActiveClientID() );
    return updateServer + "/" + client + "/" + version + "/";
}

QString Settings::getLibsUrl() const
{
    return updateServer + "/libraries/";
}

QString Settings::getAssetsUrl() const
{
    return updateServer + "/assets/";
}

QStringList Settings::getClientCaptions() const
{
    return clients.values();
}

QStringList Settings::getClientNames() const
{
    return clients.keys();
}

int Settings::getClientID(const QString &strid) const
{
    return clients.keys().indexOf(strid);
}

QString Settings::getClientCaption(int index) const
{
    QString name = getClientName(index);

    if ( clients.contains(name) )
    {
        return clients[name];
    }

    return "Unknown client";
}

QString Settings::getClientName(int index) const
{
    if (index < 0 || clients.size() <= index)
    {
        return "unknown";
    }

    return clients.keys()[index];
}

int Settings::loadActiveClientID() const
{
    QString strid = settings->value("launcher/client", "default").toString();
    return getClientID(strid);
}

void Settings::saveActiveClientID(int id) const
{
    QString client = getClientName(id);
    settings->setValue("launcher/client", client);
}

QString Settings::loadLogin() const
{
    return settings->value("launcher/login", "Player").toString();
}

void Settings::saveLogin(const QString &login) const
{
    settings->setValue("launcher/login", login);
}

bool Settings::loadPassStoreState() const
{
    return settings->value("launcher/save_password", false).toBool();
}

void Settings::savePassStoreState(bool state) const
{
    settings->setValue("launcher/save_password", state);
}

// Password stored as base64-encoded string
QString Settings::loadPassword() const
{
    QString encodedPass = settings->value("launcher/password", "").toString();
    QByteArray ba;
    ba.append(encodedPass);
    return QByteArray::fromBase64(ba);
}

void Settings::savePassword(const QString &password) const
{
    QByteArray ba;
    ba.append(password);
    settings->setValue( "launcher/password", QString( ba.toBase64() ) );
}

QString Settings::makeMinecraftUuid() const
{
    QString def = QString( QUuid::createUuid().toByteArray().toBase64() );
    QString id = settings->value("launcher/revision", def).toString();
    settings->setValue("launcher/revision", id);
    QByteArray encoded;
    encoded.append(id);
    return QString( QByteArray::fromBase64(encoded) );
}

// Minecraft window geometry
QRect Settings::loadClientWindowGeometry() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_custom";

    QVariant rect = settings->value( entry, QRect(-1, -1, 854, 480) );
    return qvariant_cast<QRect>(rect);
}

void Settings::saveClientWindowGeometry(const QRect &g) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_custom";

    settings->setValue(entry, g);
}

bool Settings::loadClientWindowSizeState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_set";
    return settings->value(entry, false).toBool();
}

void Settings::saveClientWindowSizeState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_set";
    settings->setValue(entry, state);
}

bool Settings::loadClientUseLauncherSizeState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_from_launcher";

    return settings->value(entry, false).toBool();
}

void Settings::saveClientUseLauncherSizeState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_from_launcher";

    return settings->setValue(entry, state);
}

bool Settings::loadClientFullscreenState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_fullscreen";

    return settings->value(entry, false).toBool();
}

void Settings::saveClientFullscreenState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_fullscreen";

    settings->setValue(entry, state);
}

// Launcher window geometry
QRect Settings::loadWindowGeometry() const
{
    QRect def = QRect(-1, -1, 600, 400);
    QVariant value = settings->value("launcher/window_geometry", def);

    return qvariant_cast<QRect>(value);
}

void Settings::saveWindowGeometry(const QRect &geom) const
{
    settings->setValue("launcher/window_geometry", geom);
}

bool Settings::loadMaximizedState() const
{
    return settings->value("launcher/window_maximized", false).toBool();
}

void Settings::saveMaximizedState(bool state) const
{
    settings->setValue("launcher/window_maximized", state);
}

bool Settings::loadOfflineModeState() const
{
    return settings->value("launcher/offline_mode", false).toBool();
}

void Settings::saveOfflineModeState(bool offlineState) const
{
    settings->setValue("launcher/offline_mode", offlineState);
}

bool Settings::loadHideWindowModeState() const
{
    return settings->value("launcher/hide_on_run", true).toBool();
}

void Settings::saveHideWindowModeState(bool hideState) const
{
    settings->setValue("launcher/hide_on_run", hideState);
}

// Client settings
QString Settings::loadClientVersion() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/version";

    return settings->value(entry, "latest").toString();
}

void Settings::saveClientVersion(const QString &version) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/version";

    settings->setValue(entry, version);
}

bool Settings::loadClientJavaState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/custom_java_enabled";

    return settings->value(entry, false).toBool();
}

void Settings::saveClientJavaState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/custom_java_enabled";

    settings->setValue(entry, state);
}

QString Settings::loadClientJava() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/custom_java";

    return settings->value(entry, "").toString();
}

void Settings::saveClientJava(const QString &java) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/custom_java";

    settings->setValue(entry, java);
}

bool Settings::loadClientJavaArgsState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/cutsom_args_enabled";

    return settings->value(entry, false).toBool();
}

void Settings::saveClientJavaArgsState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/cutsom_args_enabled";

    settings->setValue(entry, state);
}

QString Settings::loadClientJavaArgs() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/cutsom_args";

    return settings->value(entry, "").toString();
}

void Settings::saveClientJavaArgs(const QString &args) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/cutsom_args";

    settings->setValue(entry, args);
}

bool Settings::loadClientJavaKeystoreState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_enabled";

    return settings->value(entry, false).toBool();
}

void Settings::saveClientJavaKeystoreState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_enabled";

    settings->setValue(entry, state);
}

QString Settings::loadClientJavaKeystorePath() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_path";

    return settings->value(entry, "").toString();
}

void Settings::saveClientJavaKeystorePath(const QString &path) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_path";

    settings->setValue(entry, path);
}

QString Settings::loadClientJavaKeystorePass() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_password";

    return settings->value(entry, "").toString();
}

void Settings::saveClientJavaKeystorePass(const QString &pass) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_password";

    settings->setValue(entry, pass);
}

bool Settings::loadClientCheckAssetsState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/check_assets";

    return settings->value(entry, true).toBool();
}

void Settings::saveClientCheckAssetsState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/check_assets";

    settings->setValue(entry, state);
}

// Local store settings
QString Settings::loadStoreExePath() const
{
    QString entry = "localstore/exe_path";
    QString defaultPath = "ttyhstore";

    return settings->value(entry, defaultPath).toString();
}

void Settings::saveStoreExePath(const QString &path) const
{
    QString entry = "localstore/exe_path";
    settings->setValue(entry, path);
}

QString Settings::loadStoreDirPath() const
{
    QString entry = "localstore/dir_path";
    QString defaultPath = QDir::homePath();

    return settings->value(entry, defaultPath).toString();
}

void Settings::saveStoreDirPath(const QString &path) const
{
    QString entry = "localstore/dir_path";
    settings->setValue(entry, path);
}

// News
bool Settings::loadNewsState() const
{
    return settings->value("launcher/load_news", true).toBool();
}

void Settings::saveNewsState(bool state) const
{
    settings->setValue("launcher/load_news", state);
}

// Directories
QString Settings::getBaseDir() const
{
    return dataPath;
}

QString Settings::getClientDir() const
{
    return dataPath + "/client_" + getClientName( loadActiveClientID() );
}

QString Settings::getClientPrefix(const QString &version) const
{
    return getClientDir() + "/prefixes/" + version;
}

QString Settings::getAssetsDir() const
{
    return dataPath + "/assets";
}

QString Settings::getLibsDir() const
{
    return dataPath + "/libraries";
}

QString Settings::getVersionsDir() const
{
    return getClientDir() + "/versions";
}

QString Settings::getNativesDir() const
{
    return getClientDir() + "/natives";
}

QString Settings::getConfigDir() const
{
    return configPath;
}

// Platform information
QString Settings::getOsName() const
{
#ifdef Q_OS_WIN
    return "windows";
#endif
#ifdef Q_OS_OSX
    return "osx";
#endif
#ifdef Q_OS_LINUX
    return "linux";
#endif
}

QString Settings::getOsVersion() const
{
#ifdef Q_OS_WIN
    switch (QSysInfo::WindowsVersion)
    {
    case QSysInfo::WV_95:
        return "95, OMFG!";
    case QSysInfo::WV_98:
        return "98";
    case QSysInfo::WV_Me:
        return "Me";
    case QSysInfo::WV_NT:
        return "NT";
    case QSysInfo::WV_2000:
        return "2000";
    case QSysInfo::WV_XP:
        return "XP";
    case QSysInfo::WV_2003:
        return "2003";
    case QSysInfo::WV_VISTA:
        return "Vista";
    case QSysInfo::WV_WINDOWS7:
        return "7";
    case QSysInfo::WV_WINDOWS8:
        return "8";
    case QSysInfo::WV_WINDOWS8_1:
        return "8.1";
    case QSysInfo::WV_WINDOWS10:
        return "10";
    default:
        return "unknown";
    }
#endif

#ifdef Q_OS_OSX
    switch (QSysInfo::MacintoshVersion)
    {
    case QSysInfo::MV_10_6:
        return "10.6";
    case QSysInfo::MV_10_7:
        return "10.7";
    case QSysInfo::MV_10_8:
        return "10.8";
    case QSysInfo::MV_10_9:
        return "10.9";
    case QSysInfo::MV_10_10:
        return "10.10";
    case QSysInfo::MV_10_11:
        return "10.11";
    default:
        return "unknown";
    }
#endif

#ifdef Q_OS_LINUX
    QProcess lsbRelease;
    lsbRelease.start("lsb_release", QStringList() << "-d");

    if ( !lsbRelease.waitForStarted() )
    {
        return "NO_LSB_DISTRO";
    }
    if ( !lsbRelease.waitForFinished() )
    {
        return "ERROR";
    }

    // Get value from output: "Description:\t<value>\n"
    QString releaseInfo = lsbRelease.readLine().split('\t').last();
    releaseInfo = releaseInfo.split('\n').first();

    return releaseInfo;
#endif
}

QString Settings::getWordSize() const
{
    return QString::number(QSysInfo::WordSize);
}

QNetworkAccessManager *Settings::getNetworkAccessManager() const
{
    return nam;
}
