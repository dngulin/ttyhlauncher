#include <config.h>

#include <QtCore>
#include <QStandardPaths>
#include <QSysInfo>

#include "oldsettings.h"
#include "filefetcher.h"
#include "jsonparser.h"

#include "util.h"

typedef QStandardPaths Path;

const QString OldSettings::launcherVersion = PROJECT_VERSION;

const QString OldSettings::newsFeed = "https://ttyh.ru/misc.php?page=feed";

const QString OldSettings::updateServer = "http://store.ttyh.ru";
const QString OldSettings::buildServer = "https://ttyh.ru/builds";

// Master-server links
const QString OldSettings::master = "https://master.ttyh.ru/index.php";
const QString OldSettings::authUrl = master + "?act=login";
const QString OldSettings::changePasswrdUrl = master + "?act=chpass";
const QString OldSettings::skinUploadUrl = master + "?act=setskin";
const QString OldSettings::feedbackUrl = master + "?act=feedback";

const int OldSettings::timeout = 3000;

OldSettings *OldSettings::myInstance = NULL;
OldSettings *OldSettings::instance()
{
    if (myInstance == NULL)
    {
        myInstance = new OldSettings();
    }
    return myInstance;
}

OldSettings::OldSettings() : QObject()
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

void OldSettings::log(const QString &text)
{
    OldLogger::logger()->appendLine(tr("Settings"), text);
}

void OldSettings::updateLocalData()
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

void OldSettings::fetchLatestVersion()
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

QString OldSettings::getlatestVersion() const
{
    return latestVersion;
}

QString OldSettings::getVersionsUrl() const
{
    QString client = getClientName( loadActiveClientID() );
    return updateServer + "/" + client + "/versions/versions.json";
}

QString OldSettings::getVanillaVersionsUrl() const
{
    return QString(
        "http://s3.amazonaws.com/Minecraft.Download/versions/versions.json");
}

QString OldSettings::getVersionUrl(const QString &version)
{
    QString client = getClientName( loadActiveClientID() );
    return updateServer + "/" + client + "/" + version + "/";
}

QString OldSettings::getLibsUrl() const
{
    return updateServer + "/libraries/";
}

QString OldSettings::getAssetsUrl() const
{
    return updateServer + "/assets/";
}

QStringList OldSettings::getClientCaptions() const
{
    return clients.values();
}

QStringList OldSettings::getClientNames() const
{
    return clients.keys();
}

int OldSettings::getClientID(const QString &strid) const
{
    return clients.keys().indexOf(strid);
}

QString OldSettings::getClientCaption(int index) const
{
    QString name = getClientName(index);

    if ( clients.contains(name) )
    {
        return clients[name];
    }

    return "Unknown client";
}

QString OldSettings::getClientName(int index) const
{
    if (index < 0 || clients.size() <= index)
    {
        return "unknown";
    }

    return clients.keys()[index];
}

int OldSettings::loadActiveClientID() const
{
    QString strid = settings->value("launcher/client", "default").toString();
    return getClientID(strid);
}

void OldSettings::saveActiveClientID(int id) const
{
    QString client = getClientName(id);
    settings->setValue("launcher/client", client);
}

QString OldSettings::loadLogin() const
{
    return settings->value("launcher/login", "Player").toString();
}

void OldSettings::saveLogin(const QString &login) const
{
    settings->setValue("launcher/login", login);
}

bool OldSettings::loadPassStoreState() const
{
    return settings->value("launcher/save_password", false).toBool();
}

void OldSettings::savePassStoreState(bool state) const
{
    settings->setValue("launcher/save_password", state);
}

// Password stored as base64-encoded string
QString OldSettings::loadPassword() const
{
    QString encodedPass = settings->value("launcher/password", "").toString();
    QByteArray ba;
    ba.append(encodedPass);
    return QByteArray::fromBase64(ba);
}

void OldSettings::savePassword(const QString &password) const
{
    QByteArray ba;
    ba.append(password);
    settings->setValue( "launcher/password", QString( ba.toBase64() ) );
}

QString OldSettings::makeMinecraftUuid() const
{
    QString def = QString( QUuid::createUuid().toByteArray().toBase64() );
    QString id = settings->value("launcher/revision", def).toString();
    settings->setValue("launcher/revision", id);
    QByteArray encoded;
    encoded.append(id);
    return QString( QByteArray::fromBase64(encoded) );
}

// Minecraft window geometry
QRect OldSettings::loadClientWindowGeometry() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_custom";

    QVariant rect = settings->value( entry, QRect(-1, -1, 854, 480) );
    return qvariant_cast<QRect>(rect);
}

void OldSettings::saveClientWindowGeometry(const QRect &g) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_custom";

    settings->setValue(entry, g);
}

bool OldSettings::loadClientWindowSizeState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_set";
    return settings->value(entry, false).toBool();
}

void OldSettings::saveClientWindowSizeState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_set";
    settings->setValue(entry, state);
}

bool OldSettings::loadClientUseLauncherSizeState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_from_launcher";

    return settings->value(entry, false).toBool();
}

void OldSettings::saveClientUseLauncherSizeState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_from_launcher";

    return settings->setValue(entry, state);
}

bool OldSettings::loadClientFullscreenState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_fullscreen";

    return settings->value(entry, false).toBool();
}

void OldSettings::saveClientFullscreenState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/window_geometry_fullscreen";

    settings->setValue(entry, state);
}

// Launcher window geometry
QRect OldSettings::loadWindowGeometry() const
{
    QRect def = QRect(-1, -1, 600, 400);
    QVariant value = settings->value("launcher/window_geometry", def);

    return qvariant_cast<QRect>(value);
}

void OldSettings::saveWindowGeometry(const QRect &geom) const
{
    settings->setValue("launcher/window_geometry", geom);
}

bool OldSettings::loadMaximizedState() const
{
    return settings->value("launcher/window_maximized", false).toBool();
}

void OldSettings::saveMaximizedState(bool state) const
{
    settings->setValue("launcher/window_maximized", state);
}

bool OldSettings::loadOfflineModeState() const
{
    return settings->value("launcher/offline_mode", false).toBool();
}

void OldSettings::saveOfflineModeState(bool offlineState) const
{
    settings->setValue("launcher/offline_mode", offlineState);
}

bool OldSettings::loadHideWindowModeState() const
{
    return settings->value("launcher/hide_on_run", true).toBool();
}

void OldSettings::saveHideWindowModeState(bool hideState) const
{
    settings->setValue("launcher/hide_on_run", hideState);
}

// Client settings
QString OldSettings::loadClientVersion() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/version";

    return settings->value(entry, "latest").toString();
}

void OldSettings::saveClientVersion(const QString &version) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/version";

    settings->setValue(entry, version);
}

bool OldSettings::loadClientJavaState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/custom_java_enabled";

    return settings->value(entry, false).toBool();
}

void OldSettings::saveClientJavaState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/custom_java_enabled";

    settings->setValue(entry, state);
}

QString OldSettings::loadClientJava() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/custom_java";

    return settings->value(entry, "").toString();
}

void OldSettings::saveClientJava(const QString &java) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/custom_java";

    settings->setValue(entry, java);
}

bool OldSettings::loadClientJavaArgsState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/cutsom_args_enabled";

    return settings->value(entry, false).toBool();
}

void OldSettings::saveClientJavaArgsState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/cutsom_args_enabled";

    settings->setValue(entry, state);
}

QString OldSettings::loadClientJavaArgs() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/cutsom_args";

    return settings->value(entry, "").toString();
}

void OldSettings::saveClientJavaArgs(const QString &args) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/cutsom_args";

    settings->setValue(entry, args);
}

bool OldSettings::loadClientJavaKeystoreState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_enabled";

    return settings->value(entry, false).toBool();
}

void OldSettings::saveClientJavaKeystoreState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_enabled";

    settings->setValue(entry, state);
}

QString OldSettings::loadClientJavaKeystorePath() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_path";

    return settings->value(entry, "").toString();
}

void OldSettings::saveClientJavaKeystorePath(const QString &path) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_path";

    settings->setValue(entry, path);
}

QString OldSettings::loadClientJavaKeystorePass() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_password";

    return settings->value(entry, "").toString();
}

void OldSettings::saveClientJavaKeystorePass(const QString &pass) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/java_keystore_password";

    settings->setValue(entry, pass);
}

bool OldSettings::loadClientCheckAssetsState() const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/check_assets";

    return settings->value(entry, true).toBool();
}

void OldSettings::saveClientCheckAssetsState(bool state) const
{
    QString client = getClientName( loadActiveClientID() );
    QString entry = "client-" + client + "/check_assets";

    settings->setValue(entry, state);
}

// Local store settings
QString OldSettings::loadStoreExePath() const
{
    QString entry = "localstore/exe_path";
    QString defaultPath = "ttyhstore";

    return settings->value(entry, defaultPath).toString();
}

void OldSettings::saveStoreExePath(const QString &path) const
{
    QString entry = "localstore/exe_path";
    settings->setValue(entry, path);
}

QString OldSettings::loadStoreDirPath() const
{
    QString entry = "localstore/dir_path";
    QString defaultPath = QDir::homePath();

    return settings->value(entry, defaultPath).toString();
}

void OldSettings::saveStoreDirPath(const QString &path) const
{
    QString entry = "localstore/dir_path";
    settings->setValue(entry, path);
}

// News
bool OldSettings::loadNewsState() const
{
    return settings->value("launcher/load_news", true).toBool();
}

void OldSettings::saveNewsState(bool state) const
{
    settings->setValue("launcher/load_news", state);
}

// Directories
QString OldSettings::getBaseDir() const
{
    return dataPath;
}

QString OldSettings::getClientDir() const
{
    return dataPath + "/client_" + getClientName( loadActiveClientID() );
}

QString OldSettings::getClientPrefix(const QString &version) const
{
    return getClientDir() + "/prefixes/" + version;
}

QString OldSettings::getAssetsDir() const
{
    return dataPath + "/assets";
}

QString OldSettings::getLibsDir() const
{
    return dataPath + "/libraries";
}

QString OldSettings::getVersionsDir() const
{
    return getClientDir() + "/versions";
}

QString OldSettings::getNativesDir() const
{
    return getClientDir() + "/natives";
}

QString OldSettings::getConfigDir() const
{
    return configPath;
}

// Platform information
QString OldSettings::getOsName() const
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

QString OldSettings::getOsVersion() const
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

QString OldSettings::getWordSize() const
{
    return QString::number(QSysInfo::WordSize);
}

QNetworkAccessManager *OldSettings::getNetworkAccessManager() const
{
    return nam;
}
