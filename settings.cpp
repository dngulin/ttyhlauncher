#include <QtCore>
#include <QStandardPaths>
#include <QSysInfo>


#include "settings.h"
#include "util.h"

/*
 * Versions feature plan:
 *     - 0.1 -- implemented settings;
 *     - 0.2 -- implemented interaction with master-server;
 *     - 0.3 -- implemented interaction with update-server;
 *     - 0.4 -- implemented game launch;
 *     - 0.5 -- implemented logger, some bugfixes, code cleanups;
 *     - 0.6 -- ????
 *     - 1.0 -- PROFIT^WRELEASE!
*/
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

void Settings::loadClientList() {

    Logger* logger = Logger::logger();

    QFile* prefixesFile = new QFile(dataPath + "/prefixes.json");

    logger->appendLine("Settings", "Updating local client list...");
    Reply prefixesReply = Util::makeGet(nam, updateServer + "/prefixes.json");

    if (prefixesReply.isSuccess()) {

        logger->appendLine("Settings", "OK. Saving local copy...");
        if (prefixesFile->open(QIODevice::WriteOnly)) {
            prefixesFile->write(prefixesReply.getData());
            prefixesFile->close();

        } else {
            logger->appendLine("Settings", "Error: save list: " + prefixesFile->errorString());
        }

    } else {
        logger->appendLine("Settings", "Error: " + prefixesReply.getErrorString());
    }

    logger->appendLine("Settings", "Loading local client list...");
    if (prefixesFile->open(QIODevice::ReadOnly)) {

        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(prefixesFile->readAll(), &error);

        if (error.error == QJsonParseError::NoError) {

            QJsonObject clients = json.object()["prefixes"].toObject();
            foreach (QString key, clients.keys()) {
                QJsonObject client = clients[key].toObject();
                if (client["type"] == "public") {
                    appendClient(key, client["about"].toString());
                    logger->appendLine("Settings", "Add client: " + key);
                }
            }

        } else {
            logger->appendLine("Settings", "Error: JSON: " + error.errorString() + " at " + QString::number(error.offset));
        }

        prefixesFile->close();

    } else {
        logger->appendLine("Settings", "Error: open list: " + prefixesFile->errorString());
    }

    delete prefixesFile;

}

void Settings::loadCustomKeystore() {
    Logger* logger = Logger::logger();
    QFile* keystoreFile = new QFile(configPath + "/keystore.ks");

    logger->appendLine("Settings", "Updating local java keystore...");
    Reply keystoreReply = Util::makeGet(nam, updateServer + "/store.ks");

    if (keystoreReply.isSuccess()) {

        logger->appendLine("Settings", "OK. Saving local copy...");
        if (keystoreFile->open(QIODevice::WriteOnly)) {
            keystoreFile->write(keystoreReply.getData());
            keystoreFile->close();

        } else {
            logger->appendLine("Settings", "Error: save keystore: " + keystoreFile->errorString());
        }

    } else {
        logger->appendLine("Settings", "Error: " + keystoreReply.getErrorString());
    }

    delete keystoreFile;
}

void Settings::appendClient(const QString &strid, const QString &name) {
    clientStrIDs.append(strid);
    clientNames.append(name);
}

QString Settings::getVersionsUrl() {
    QString client = getClientStrId(loadActiveClientId());
    return updateServer + "/" + client + "/versions/versions.json";
}

QString Settings::getVersionUrl(const QString &version) {
    QString client = getClientStrId(loadActiveClientId());
    return updateServer + "/" + client + "/" + version + "/";
}

QString Settings::getLibsUrl() {
    return updateServer + "/libraries/";
}

QString Settings::getAssetsUrl() {
    return updateServer + "/assets/";
}

QStringList Settings::getClientsNames() { return clientNames; }
int Settings::getClientId(QString name) { return clientNames.indexOf(name); }
int Settings::strIDtoID(QString strid) { return clientStrIDs.indexOf(strid); }

QString Settings::getClientName(int id) {
    if (id < 0) return "Unknown client";
    if (clientNames.size() <= id) return "Unknown client";
    return clientNames.at(id);
}
QString Settings::getClientStrId(int id) {
    if (id < 0) return "unknown";
    if (clientStrIDs.size() <= id) return "unknown";
    return clientStrIDs.at(id);
}

int Settings::loadActiveClientId() {
    QString strid = settings->value("launcher/client", "default").toString();
    return strIDtoID(strid);
}

void Settings::saveActiveClientId(int id) {
    QString client = getClientStrId(id);
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
    int c = loadActiveClientId();
    return qvariant_cast<QRect>(settings->value("client-" + getClientStrId(c) + "/window_geometry_custom", QRect(-1, -1, 854, 480)));
}
void Settings::saveClientWindowGeometry(const QRect &g) {
    int c = loadActiveClientId();
    settings->setValue("client-" + getClientStrId(c) + "/window_geometry_custom", g);
}

bool Settings::loadClientWindowSizeState() {
    int c = loadActiveClientId();
    return settings->value("client-" + getClientStrId(c) + "/window_geometry_set", false).toBool();
}
void Settings::saveClientWindowSizeState(bool state) {
    int c = loadActiveClientId();
    settings->setValue("client-" + getClientStrId(c) + "/window_geometry_set", state);
}

bool Settings::loadClientUseLauncherSizeState() {
    int c = loadActiveClientId();
    return settings->value("client-" + getClientStrId(c) + "/window_geometry_from_launcher", false).toBool();
}
void Settings::saveClientUseLauncherSizeState(bool state) {
    int c = loadActiveClientId();
    return settings->setValue("client-" + getClientStrId(c) + "/window_geometry_from_launcher", state);
}

bool Settings::loadClientFullscreenState() {
    int c = loadActiveClientId();
    return settings->value("client-" + getClientStrId(c) + "/window_geometry_fullscreen", false).toBool();
}
void Settings::saveClientFullscreenState(bool state) {
    int c = loadActiveClientId();
    settings->setValue("client-" + getClientStrId(c) + "/window_geometry_fullscreen", state);
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
    int cid = loadActiveClientId();
    return settings->value("client-" + getClientStrId(cid) + "/version", "latest").toString();
}

void Settings::saveClientVersion(const QString &strid) {
    int cid = loadActiveClientId();
    settings->setValue("client-" + getClientStrId(cid) + "/version", strid);
}

bool Settings::loadClientJavaState() {
    int cid = loadActiveClientId();
    return settings->value("client-" + getClientStrId(cid) + "/custom_java_enabled", false).toBool();
}

void Settings::saveClientJavaState(bool state) {
    int cid = loadActiveClientId();
    settings->setValue("client-" + getClientStrId(cid) + "/custom_java_enabled", state);
}

QString Settings::loadClientJava() {
    int cid = loadActiveClientId();
    return settings->value("client-" + getClientStrId(cid) + "/custom_java", "").toString();
}

void Settings::saveClientJava(const QString &java) {
    int cid = loadActiveClientId();
    settings->setValue("client-" + getClientStrId(cid) + "/custom_java", java);
}

bool Settings::loadClientJavaArgsState() {
    int cid = loadActiveClientId();
    return settings->value("client-" + getClientStrId(cid) + "/cutsom_args_enabled", false).toBool();
}

void Settings::saveClientJavaArgsState(bool state) {
    int cid = loadActiveClientId();
    settings->setValue("client-" + getClientStrId(cid) + "/cutsom_args_enabled", state);
}

QString Settings::loadClientJavaArgs() {
    int cid = loadActiveClientId();
    return settings->value("client-" + getClientStrId(cid) + "/cutsom_args", "").toString();
}

void Settings::saveClientJavaArgs(const QString &args) {
    int cid = loadActiveClientId();
    settings->setValue("client-" + getClientStrId(cid) + "/cutsom_args", args);
}

bool Settings::loadClientCheckAssetsState()
{
    int cid = loadActiveClientId();
    return settings->value("client-" + getClientStrId(cid) + "/check_assets", true).toBool();
}
void Settings::saveClientCheckAssetsState(bool state)
{
    int cid = loadActiveClientId();
    settings->setValue("client-" + getClientStrId(cid) + "/check_assets", state);
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
    return dataPath + "/client_" + getClientStrId(loadActiveClientId());
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
