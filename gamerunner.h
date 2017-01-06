#ifndef GAMERUNNER_H
#define GAMERUNNER_H

#include <QObject>

#include "settings.h"
#include "logger.h"
#include "datafetcher.h"
#include "filefetcher.h"
#include "jsonparser.h"
#include "hashchecker.h"
#include "fileinfo.h"
#include "libraryinfo.h"

class GameRunner : public QObject
{
    Q_OBJECT
public:
    GameRunner(const QString &login, const QString &pass, bool onlineMode,
               const QRect &windowGeometry, QObject *parent = 0);
    ~GameRunner();

    void Run();

signals:
    void error(const QString &message);
    void needUpdate(const QString &message);
    void started();
    void finished(int exitCode);

    void beginCheck(const QList<FileInfo> list, bool stopOnBad);

private:
    // Initial data
    QString name;
    QString password;

    QRect geometry;
    bool isOnline;

    QString version;

    // Login data
    QString clientToken;
    QString accessToken;

    // Checking urls and pathes in online mode
    QString versionIndexUrl;
    QString versionIndexPath;
    QString dataIndexUrl;
    QString dataIndexPath;
    QString assetsIndexUrl;
    QString assetsIndexPath;

    // Checking data
    QThread checkThread;
    HashChecker *checker;

    QList<FileInfo> checkList;

    // Run data
    JsonParser versionParser;
    QProcess minecraft;

    // Additional data
    Settings *settings;
    Logger *logger;
    DataFetcher fetcher;

    // Logging
    void log(const QString &text);

    // Run steps
    void requestAcessToken();
    void determinateVersion();

    void checkIndexes();
    void requestVersionIndex();
    void requestDataIndex();
    void requestAssetsIndex();

    void checkFiles();
    void runGame();

    void readVersionIndexInfo(const QString &indexName);
    void emitError(const QString &message);
    void emitNeedUpdate(const QString &message);

private slots:
    void acessTokenReceived(bool result);
    void versionsListReceived(bool result);

    void versionIndexReceived(bool result);
    void dataIndexReceived(bool result);
    void assetsIndexReceived(bool result);

    void onBadChecksum(const FileInfo fileInfo);

    void gameLog();
    void onGameError(QProcess::ProcessError error);
    void onGameStarted();
    void onGameFinished(int exitCode);
};

#endif // GAMERUNNER_H
