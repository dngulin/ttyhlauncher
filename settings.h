#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore>
#include <QNetworkAccessManager>

class Settings : public QObject
{
    Q_OBJECT
public:
    static Settings *instance();

    static const QString authUrl;
    static const QString changePasswrdUrl;
    static const QString skinUploadUrl;
    static const QString feedbackUrl;

    static const QString launcherVersion;

    static const QString newsFeed;

    static const QString updateServer;
    static const QString buildServer;

    static const int timeout;

private:
    static const QString master;
    static Settings *myInstance;

private:
    Settings();
    QSettings *settings;

    QNetworkAccessManager *nam;

    QHash<QString, QString> clients; // str_id, title

    QString dataPath;
    QString configPath;

    QString latestVersion;

    void log(const QString &text);

public:
    // Update URLs
    QString getVersionsUrl() const;
    QString getVanillaVersionsUrl() const;
    QString getVersionUrl(const QString &version);
    QString getLibsUrl() const;
    QString getAssetsUrl() const;

    void updateLocalData();
    void fetchLatestVersion();

    QString getlatestVersion() const;

    // Clients
    QStringList getClientCaptions() const;
    int getClientID(const QString &strid) const;
    QString getClientName(int index) const;
    QString getClientCaption(int index) const;

    QStringList getClientNames() const;

    // Directories
    QString getBaseDir() const;
    QString getClientDir() const;
    QString getClientPrefix(const QString &version) const;
    QString getAssetsDir() const;
    QString getLibsDir() const;
    QString getVersionsDir() const;
    QString getNativesDir() const;
    QString getConfigDir() const;

    // Launcher settings
    int loadActiveClientID() const;
    QString loadLogin() const;
    bool loadPassStoreState() const;
    bool loadMaximizedState() const;

    QString loadPassword() const;
    void savePassword(const QString &password) const;

    QRect loadWindowGeometry() const;
    void saveWindowGeometry(const QRect &geom) const;

    bool loadOfflineModeState() const;
    void saveOfflineModeState(bool offlineState) const;

    bool loadHideWindowModeState() const;
    void saveHideWindowModeState(bool hideState) const;

    bool loadNewsState() const;
    void saveNewsState(bool state) const;

    // Client settings
    QString loadClientVersion() const;
    void saveClientVersion(const QString &version) const;

    bool loadClientJavaState() const;
    void saveClientJavaState(bool state) const;

    QString loadClientJava() const;
    void saveClientJava(const QString &java) const;

    bool loadClientJavaArgsState() const;
    void saveClientJavaArgsState(bool state) const;

    QString loadClientJavaArgs() const;
    void saveClientJavaArgs(const QString &args) const;

    bool loadClientWindowSizeState() const;
    void saveClientWindowSizeState(bool s) const;

    QRect loadClientWindowGeometry() const;
    void saveClientWindowGeometry(const QRect &g) const;

    bool loadClientFullscreenState() const;
    void saveClientFullscreenState(bool state) const;

    bool loadClientUseLauncherSizeState() const;
    void saveClientUseLauncherSizeState(bool state) const;

    bool loadClientCheckAssetsState() const;
    void saveClientCheckAssetsState(bool state) const;

    // Local TtyhStore settings
    QString loadStoreExePath() const;
    void saveStoreExePath(const QString &path) const;

    QString loadStoreDirPath() const;
    void saveStoreDirPath(const QString &path) const;

    // Custom
    QString makeMinecraftUuid() const;

    QString getOsName() const;
    QString getOsVersion() const;
    QString getWordSize() const;

    QNetworkAccessManager *getNetworkAccessManager() const;

public slots:
    void saveActiveClientID(int id) const;
    void saveLogin(const QString &login) const;
    void savePassStoreState(bool state) const;
    void saveMaximizedState(bool state) const;

private:
    Settings &operator=(Settings const &);
    Settings(Settings const &);
};

#endif // SETTINGS_H
