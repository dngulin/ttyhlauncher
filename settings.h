#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore>

#include "logger.h"

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

    QStringList clientStrIDs;
    QStringList clientNames;
    void appendClient(QString strid, QString name);

    QString dataPath;
    QString configPath;
    QString updateServer;

public:
    // Update URLs
    QString getVersionsUrl();
    QString getVersionUrl(QString version);
    QString getLibsUrl();
    QString getAssetsUrl();

    // Clients
    void loadClientList();

    // Keystore
    void loadCustomKeystore();

    QStringList getClientsNames();
    int getClientId(QString name);
    int strIDtoID(QString strid);
    QString getClientStrId(int id);
    QString getClientName(int id);

    // Directories
    QString getBaseDir();
    QString getClientDir();
    QString getClientPrefix(QString version);
    QString getAssetsDir();
    QString getLibsDir();
    QString getVersionsDir();
    QString getNativesDir();
    QString getConfigDir();

    // Launcher settings
    int loadActiveClientId();
    QString loadLogin();
    bool loadPassStoreState();
    bool loadMaximizedState();
    // save-pairs in slots section

    QString loadPassword();
    void savePassword(QString password);

    QRect loadWindowGeometry();
    void saveWindowGeometry(QRect geom);

    bool loadOfflineModeState();
    void saveOfflineModeState(bool offlineState);

    bool loadHideWindowModeState();
    void saveHideWindowModeState(bool hideState);

    int loadNewsId();
    void saveNewsId(int i);

    // Client settings
    QString loadClientVersion();
    void saveClientVersion(QString strid);

    bool loadClientJavaState();
    void saveClientJavaState(bool state);

    QString loadClientJava();
    void saveClientJava(QString java);

    bool loadClientJavaArgsState();
    void saveClientJavaArgsState(bool state);

    QString loadClientJavaArgs();
    void saveClientJavaArgs(QString args);

    bool loadClientWindowSizeState();
    void saveClientWindowSizeState(bool s);

    QRect loadClientWindowGeometry();
    void saveClientWindowGeometry(QRect g);

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

public slots:
    void saveActiveClientId(int id);
    void saveLogin(QString login);
    void savePassStoreState(bool state);
    void saveMaximizedState(bool state);


private:
    Settings& operator=(Settings const&);
    Settings(Settings const&);

};


#endif // SETTINGS_H
