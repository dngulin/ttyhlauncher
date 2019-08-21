#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include "logs/logger.h"
#include "settings/settingsmanager.h"
#include "profiles/profilesmanager.h"
#include "profiles/profilerunner.h"
#include "versions/versionsmanager.h"
#include "master/ttyhclient.h"
#include "storage/filechecker.h"
#include "storage/downloader.h"
#include "ui/mainwindow.h"

namespace Ttyh {
using namespace Ttyh::Logs;
using namespace Ttyh::Settings;
using namespace Ttyh::Versions;
using namespace Ttyh::Profiles;
using namespace Ttyh::Master;
using namespace Ttyh::Storage;

class Launcher : public QObject
{
    enum class Task { Nothing, Checking, Downloading };

    Q_OBJECT
public:
    Launcher(QSharedPointer<SettingsManager> settings, QSharedPointer<ProfilesManager> profiles,
             QSharedPointer<VersionsManager> versions, QSharedPointer<TtyhClient> client,
             QSharedPointer<FileChecker> checker, QSharedPointer<Downloader> downloader,
             QSharedPointer<ProfileRunner> runner, const QSharedPointer<Logger> &logger);

    void start();

private:
    QSharedPointer<SettingsManager> settings;
    QSharedPointer<ProfilesManager> profiles;
    QSharedPointer<VersionsManager> versions;
    QSharedPointer<TtyhClient> client;
    QSharedPointer<FileChecker> checker;
    QSharedPointer<Downloader> downloader;
    QSharedPointer<ProfileRunner> runner;
    QSharedPointer<MainWindow> window;
    NamedLogger log;

    Task activeTask;

    void showTask(Task task);
    void hideTask();

    void tryBecomeOnline();

    void connectWindowEvents();
    void connectTaskEvents();
    void connectOnlineModeFlow();
    void connectRunGameFlow();
    void connectSkinUpload();

    void loadWindowState();
    void saveWindowState();

    static QString taskToString(Task task);
    static QString getTaskTitle(Task task);

    static QString getRequestResultMessage(RequestResult result);
};
}

#endif // LAUNCHER_H
