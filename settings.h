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

    // Mainwindow login form
    QString loadLogin();
    bool loadPassStore();
    QString loadPassword();
    void savePassword(QString password);
    int loadActiveClientId();

    // Window parameters
    QRect loadWindowGeometry();
    void saveWindowGeometry(QRect geom);
    bool loadMaximizedState();

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

    QString makeMinecraftUuid();

    QString getOsName();
    QString getOsVersion();
    QString getWordSize();

public slots:
    void saveActiveClientId(int id);
    void saveLogin(QString login);
    void savePassStore(bool state);
    void saveMaximizedState(bool state);


private:
    Settings& operator=(Settings const&);
    Settings(Settings const&);

};


#endif // SETTINGS_H
