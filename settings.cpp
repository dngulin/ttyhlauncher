#include <QtCore>
#include <QStandardPaths>
#include <QSysInfo>

#include "settings.h"
#include "filefetcher.h"
#include "jsonparser.h"

#include "util.h"


const QString Settings::launcherVersion = "0.9";

// Master-server links
const QString Settings::authUrl          = "https://master.ttyh.ru/index.php?act=login";
const QString Settings::changePasswrdUrl = "https://master.ttyh.ru/index.php?act=chpass";
const QString Settings::skinUploadUrl    = "https://master.ttyh.ru/index.php?act=setskin";
const QString Settings::feedbackUrl      = "https://master.ttyh.ru/index.php?act=feedback";

Settings* Settings::myInstance = 0;
Settings* Settings::instance() {
    if (myInstance == 0) myInstance = new Settings();
    return myInstance;
}

Settings::Settings() : QObject()
{
    nam = new QNetworkAccessManager(this);

    updateServer = "http://store.ttyh.ru";

    dataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/ttyh_minecraft";
    configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/ttyhlauncher";

    // Prepare data and config directories
    QDir(dataPath).mkpath(dataPath);
    QDir(configPath).mkpath(configPath);

    settings = new QSettings(configPath + "/config.ini", QSettings::IniFormat);
}

void Settings::log(const QString &text)
{
    Logger::logger()->appendLine( tr("Settings"), text );
}

void Settings::updateLocalData() {

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
        log( tr("Error: ") + parser.getParserError() );
    }
}

QString Settings::getVersionsUrl() {
    QString client = getClientName(loadActiveClientID());
    return updateServer + "/" + client + "/versions/versions.json";
}

QString Settings::getVersionUrl(const QString &version) {
    QString client = getClientName(loadActiveClientID());
    return updateServer + "/" + client + "/" + version + "/";
}

QString Settings::getLibsUrl() {
    return updateServer + "/libraries/";
}

QString Settings::getAssetsUrl() {
    return updateServer + "/assets/";
}

QStringList Settings::getClientCaptions()
{
    return clients.values();
}

int Settings::getClientID(QString strid)
{
    return clients.keys().indexOf(strid);
}

QString Settings::getClientCaption(int index)
{
    QString name = getClientName(index);

    if ( clients.contains(name) )
    {
        return clients[name];
    }

    return "Unknown client";
}
QString Settings::getClientName(int index)
{
    if (index < 0 || clients.size() <= index) return "unknown";

    return clients.keys()[index];
}

int Settings::loadActiveClientID() {
    QString strid = settings->value("launcher/client", "default").toString();
    return getClientID(strid);
}

void Settings::saveActiveClientID(int id) {
    QString client = getClientName(id);
    settings->setValue("launcher/client", client);
}

QString Settings::loadLogin() { return settings->value("launcher/login", "Player").toString(); }
void Settings::saveLogin(const QString &login) { settings->setValue("launcher/login", login); }

bool Settings::loadPassStoreState() { return settings->value("launcher/save_password", false).toBool(); }
void Settings::savePassStoreState(bool state) { settings->setValue("launcher/save_password", state); }

// Password stored as base64-encoded string
QString Settings::loadPassword() {
    QString encodedPass = settings->value("launcher/password", "").toString();
    QByteArray ba; ba.append(encodedPass);
    return QByteArray::fromBase64(ba);
}

void Settings::savePassword(const QString &password) {
    QByteArray ba; ba.append(password);
    settings->setValue("launcher/password", QString(ba.toBase64()));
}

QString Settings::makeMinecraftUuid() {
    QString def = QString(QUuid::createUuid().toByteArray().toBase64());
    QString id = settings->value("launcher/revision", def).toString();
    settings->setValue("launcher/revision", id);
    QByteArray encoded; encoded.append(id);
    return QString(QByteArray::fromBase64(encoded));
}

// Minecraft window geometry
QRect Settings::loadClientWindowGeometry() {
    int c = loadActiveClientID();
    return qvariant_cast<QRect>(settings->value("client-" + getClientName(c) + "/window_geometry_custom", QRect(-1, -1, 854, 480)));
}
void Settings::saveClientWindowGeometry(const QRect &g) {
    int c = loadActiveClientID();
    settings->setValue("client-" + getClientName(c) + "/window_geometry_custom", g);
}

bool Settings::loadClientWindowSizeState() {
    int c = loadActiveClientID();
    return settings->value("client-" + getClientName(c) + "/window_geometry_set", false).toBool();
}
void Settings::saveClientWindowSizeState(bool state) {
    int c = loadActiveClientID();
    settings->setValue("client-" + getClientName(c) + "/window_geometry_set", state);
}

bool Settings::loadClientUseLauncherSizeState() {
    int c = loadActiveClientID();
    return settings->value("client-" + getClientName(c) + "/window_geometry_from_launcher", false).toBool();
}
void Settings::saveClientUseLauncherSizeState(bool state) {
    int c = loadActiveClientID();
    return settings->setValue("client-" + getClientName(c) + "/window_geometry_from_launcher", state);
}

bool Settings::loadClientFullscreenState() {
    int c = loadActiveClientID();
    return settings->value("client-" + getClientName(c) + "/window_geometry_fullscreen", false).toBool();
}
void Settings::saveClientFullscreenState(bool state) {
    int c = loadActiveClientID();
    settings->setValue("client-" + getClientName(c) + "/window_geometry_fullscreen", state);
}

// Launcher window geometry
QRect Settings::loadWindowGeometry() {return qvariant_cast<QRect>(settings->value("launcher/window_geometry", QRect(-1, -1, 600, 400))); }
void Settings::saveWindowGeometry(const QRect &geom) { settings->setValue("launcher/window_geometry", geom); }

bool Settings::loadMaximizedState() {return settings->value("launcher/window_maximized", false).toBool();}
void Settings::saveMaximizedState(bool state) { settings->setValue("launcher/window_maximized", state); }

bool Settings::loadOfflineModeState() { return settings->value("launcher/offline_mode", false).toBool(); }
void Settings::saveOfflineModeState(bool offlineState) { settings->setValue("launcher/offline_mode", offlineState); }

bool Settings::loadHideWindowModeState() { return settings->value("launcher/hide_on_run", true).toBool(); }
void Settings::saveHideWindowModeState(bool hideState) { settings->setValue("launcher/hide_on_run", hideState); }

// Client settings
QString Settings::loadClientVersion() {
    int cid = loadActiveClientID();
    return settings->value("client-" + getClientName(cid) + "/version", "latest").toString();
}

void Settings::saveClientVersion(const QString &strid) {
    int cid = loadActiveClientID();
    settings->setValue("client-" + getClientName(cid) + "/version", strid);
}

bool Settings::loadClientJavaState() {
    int cid = loadActiveClientID();
    return settings->value("client-" + getClientName(cid) + "/custom_java_enabled", false).toBool();
}

void Settings::saveClientJavaState(bool state) {
    int cid = loadActiveClientID();
    settings->setValue("client-" + getClientName(cid) + "/custom_java_enabled", state);
}

QString Settings::loadClientJava() {
    int cid = loadActiveClientID();
    return settings->value("client-" + getClientName(cid) + "/custom_java", "").toString();
}

void Settings::saveClientJava(const QString &java) {
    int cid = loadActiveClientID();
    settings->setValue("client-" + getClientName(cid) + "/custom_java", java);
}

bool Settings::loadClientJavaArgsState() {
    int cid = loadActiveClientID();
    return settings->value("client-" + getClientName(cid) + "/cutsom_args_enabled", false).toBool();
}

void Settings::saveClientJavaArgsState(bool state) {
    int cid = loadActiveClientID();
    settings->setValue("client-" + getClientName(cid) + "/cutsom_args_enabled", state);
}

QString Settings::loadClientJavaArgs() {
    int cid = loadActiveClientID();
    return settings->value("client-" + getClientName(cid) + "/cutsom_args", "").toString();
}

void Settings::saveClientJavaArgs(const QString &args) {
    int cid = loadActiveClientID();
    settings->setValue("client-" + getClientName(cid) + "/cutsom_args", args);
}

bool Settings::loadClientCheckAssetsState()
{
    int cid = loadActiveClientID();
    return settings->value("client-" + getClientName(cid) + "/check_assets", true).toBool();
}
void Settings::saveClientCheckAssetsState(bool state)
{
    int cid = loadActiveClientID();
    settings->setValue("client-" + getClientName(cid) + "/check_assets", state);
}

// News
int Settings::loadNewsId() {
    return settings->value("launcher/news_id", 0).toInt();
}

void Settings::saveNewsId(int i) {
    settings->setValue("launcher/news_id", i);
}

// Directories
QString Settings::getBaseDir() {
    return dataPath;
}

QString Settings::getClientDir() {
    return dataPath + "/client_" + getClientName(loadActiveClientID());
}

QString Settings::getClientPrefix(const QString &version) {
    return getClientDir() + "/prefixes/" + version;
}

QString Settings::getAssetsDir() {
    return dataPath + "/assets";
}

QString Settings::getLibsDir() {
    return dataPath + "/libraries";
}

QString Settings::getVersionsDir() {
    return getClientDir() + "/versions";
}

QString Settings::getNativesDir() {
    return getClientDir() + "/natives";
}

QString Settings::getConfigDir() {
    return configPath;
}

// Platform information
QString Settings::getOsName() {
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

QString Settings::getOsVersion() {
#ifdef Q_OS_WIN
    switch (QSysInfo::WindowsVersion) {
        case QSysInfo::WV_95:           return "95, OMFG!";
        case QSysInfo::WV_98:           return "98";
        case QSysInfo::WV_Me:           return "Me";
        case QSysInfo::WV_NT:           return "NT";
        case QSysInfo::WV_2000:         return "2000";
        case QSysInfo::WV_XP:           return "XP";
        case QSysInfo::WV_2003:         return "2003";
        case QSysInfo::WV_VISTA:        return "Vista";
        case QSysInfo::WV_WINDOWS7:     return "7";
        case QSysInfo::WV_WINDOWS8:     return "8";
        case QSysInfo::WV_WINDOWS8_1:   return "8.1";
        case QSysInfo::WV_WINDOWS10:    return "10";
        default:                        return "unknown";
    }
#endif

#ifdef Q_OS_OSX
    switch (QSysInfo::MacintoshVersion) {
        case QSysInfo::MV_10_6:     return "10.6";
        case QSysInfo::MV_10_7:     return "10.7";
        case QSysInfo::MV_10_8:     return "10.8";
        case QSysInfo::MV_10_9:     return "10.9";
        default:                    return "unknown";
    }
#endif

#ifdef Q_OS_LINUX
    QProcess lsbRelease;
    lsbRelease.start("lsb_release", QStringList() << "-d");

    if (!lsbRelease.waitForStarted())  return "NO_LSB_DISTRO";
    if (!lsbRelease.waitForFinished()) return "ERROR";

    // Get value from output: "Description:\t<value>\n"
    QString releaseInfo = lsbRelease.readLine().split('\t').last();
    releaseInfo = releaseInfo.split('\n').first();

    return releaseInfo;
#endif
}

QString Settings::getWordSize() {
    return QString::number(QSysInfo::WordSize);
}

QNetworkAccessManager *Settings::getNetworkAccessManager()
{
    return nam;
}
