#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore>
#include <QNetworkAccessManager>

class Settings : public QObject
{
    Q_OBJECT
public:
    static Settings* instance();

    static const QString authUrl;
    static const QString changePasswrdUrl;
    static const QString skinUploadUrl;
    static const QString feedbackUrl;

    static const QString launcherVersion;


private:
    static Settings* myInstance;

private:
    Settings();
    QSettings* settings;

    QNetworkAccessManager* nam;

    QHash<QString,QString> clients; // str_id, title

    QString dataPath;
    QString configPath;
    QString updateServer;

    void log(const QString &text);

public:
    // Update URLs
    QString getVersionsUrl();
    QString getVersionUrl(const QString & version);
    QString getLibsUrl();
    QString getAssetsUrl();

    void updateLocalData();

    // Clients
    QStringList getClientCaptions();
    int getClientID(QString strid);
    QString getClientName(int index);
    QString getClientCaption(int index);

    // Directories
    QString getBaseDir();
    QString getClientDir();
    QString getClientPrefix(const QString & version);
    QString getAssetsDir();
    QString getLibsDir();
    QString getVersionsDir();
    QString getNativesDir();
    QString getConfigDir();

    // Launcher settings
    int loadActiveClientID();
    QString loadLogin();
    bool loadPassStoreState();
    bool loadMaximizedState();

    QString loadPassword();
    void savePassword(const QString & password);

    QRect loadWindowGeometry();
    void saveWindowGeometry(const QRect & geom);

    bool loadOfflineModeState();
    void saveOfflineModeState(bool offlineState);

    bool loadHideWindowModeState();
    void saveHideWindowModeState(bool hideState);

    int loadNewsId();
    void saveNewsId(int i);

    // Client settings
    QString loadClientVersion();
    void saveClientVersion(const QString & strid);

    bool loadClientJavaState();
    void saveClientJavaState(bool state);

    QString loadClientJava();
    void saveClientJava(const QString & java);

    bool loadClientJavaArgsState();
    void saveClientJavaArgsState(bool state);

    QString loadClientJavaArgs();
    void saveClientJavaArgs(const QString & args);

    bool loadClientWindowSizeState();
    void saveClientWindowSizeState(bool s);

    QRect loadClientWindowGeometry();
    void saveClientWindowGeometry(const QRect & g);

    bool loadClientFullscreenState();
    void saveClientFullscreenState(bool state);

    bool loadClientUseLauncherSizeState();
    void saveClientUseLauncherSizeState(bool state);

    bool loadClientCheckAssetsState();
    void saveClientCheckAssetsState(bool state);

    // Custom
    QString makeMinecraftUuid();

    QString getOsName();
    QString getOsVersion();
    QString getWordSize();

    QNetworkAccessManager *getNetworkAccessManager();

public slots:
    void saveActiveClientID(int id);
    void saveLogin(const QString & login);
    void savePassStoreState(bool state);
    void saveMaximizedState(bool state);


private:
    Settings& operator=(Settings const&);
    Settings(Settings const&);
};


#endif // SETTINGS_H
