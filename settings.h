#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore>
#include <QObject>

class Settings : public QObject
{
    Q_OBJECT
public:
    static Settings* instance();

    static const QString authUrl;
    static const QString changePasswrdUrl;
    static const QString skinUploadUrl;
    static const QString feedbackUrl;

    static const QString getVersionsUrl;
    static const QString getFilelistUrl;
    static const QString updateUrl;

    static const QString launcherVerion;


private:
    static Settings* myInstance;

private:
    Settings();
    QSettings* settings;

    QStringList* clientStrIDs;
    QStringList* clientNames;    
    void appendClient(QString strid, QString name);

    QString dataPath;

public:
    // Clients
    QStringList getClientsNames();
    int getClientId(QString name);
    int strIDtoID(QString strid);
    QString getClientStrId(int id);
    QString getClientName(int id);

    QString getClientDir();

    // Mainwindow login form
    QString loadLogin();
    bool loadPassStore();
    QString loadPassword();
    void savePassword(QString password);
    int loadActiveClientId();

    // News
    QString loadNewsSource();
    void saveNewsSource(QString id);

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
