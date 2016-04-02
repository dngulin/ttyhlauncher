#ifndef GAMERUNNER_H
#define GAMERUNNER_H

#include <QObject>

#include "settings.h"
#include "logger.h"
#include "fileinfo.h"
#include "libraryinfo.h"


class GameRunner : public QObject
{
    Q_OBJECT
public:
    GameRunner(const QString &playerLogin,
               const QString &playerPassword,
               const QString &gamePrefix,
               bool onlineMode,
               const QRect &windowGeometry,
               QObject *parent = 0);

    void startRunner();

signals:
    void runError(const QString &message);
    void needUpdate(const QString &message);
    void gameStarted();
    void gameFinished(int exitCode);

private:
    // Initial data
    QString name;
    QString password;
    QString prefix;
    QRect geometry;
    bool isOnline;

    QString gameVersion;

    // Login data
    QString clientToken;
    QString accessToken;

    // Run data form <version>.json
    QString jarName;
    QString assetesName;
    QString mainClass;
    QString minecraftArgs;
    QList<LibraryInfo> libraryList;

    // Checking data
    QList<FileInfo> checkingList;

    // Additional data
    Settings* settings;
    Logger* logger;
    QNetworkAccessManager nam;
    QProcess minecraft;

    // Run steps
    void getAccessToken();
    void findLatestVersion();
    void updateIndexes();
    void readVersionIndex();
    void makeFileChecks();
    void runGame();

    void readVersionIndexInfo(const QString& indexName);
    void emitError(const QString &message);
    void emitNeedUpdate(const QString &message);
};

#endif // GAMERUNNER_H
