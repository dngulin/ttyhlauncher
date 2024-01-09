#include "ui/profiledialog.h"
#include "ui/skindialog.h"
#include "launcher.h"

Ttyh::Launcher::Launcher(QSharedPointer<SettingsManager> settings,
                         QSharedPointer<ProfilesManager> profiles,
                         QSharedPointer<VersionsManager> versions,
                         QSharedPointer<TtyhClient> client, QSharedPointer<FileChecker> checker,
                         QSharedPointer<Downloader> downloader,
                         QSharedPointer<ProfileRunner> runner, QSharedPointer<NewsFeed> feed,
                         const QSharedPointer<Logger> &logger)
    : settings(std::move(settings)),
      profiles(std::move(profiles)),
      versions(std::move(versions)),
      client(std::move(client)),
      checker(std::move(checker)),
      downloader(std::move(downloader)),
      runner(std::move(runner)),
      feed(std::move(feed)),
      window(new MainWindow, &QObject::deleteLater),
      log(logger, "Launcher"),
      activeTask(Task::Nothing)
{
    connectWindowEvents();
    connectTaskEvents();
    connectOnlineModeFlow();
    connectRunGameFlow();
    connectProfileActions();
    connectSkinUpload();

    connect(logger.data(), &Logger::onLog, [=](const QString &msg) { window->appendLog(msg); });
}

void Ttyh::Launcher::connectWindowEvents()
{
    connect(window.data(), &MainWindow::closed, [=]() {
        switch (activeTask) {
        case Task::Checking:
            checker->cancel();
            break;
        case Task::Downloading:
            downloader->cancel();
            break;
        default:
            break;
        }

        saveWindowState();
    });

    connect(window.data(), &MainWindow::resized,
            [=](const QSize &size) { settings->data.windowSize = size; });
}

void Ttyh::Launcher::connectTaskEvents()
{
    connect(checker.data(), &FileChecker::rangeChanged,
            [=](int min, int max) { window->setTaskRange(min, max); });
    connect(checker.data(), &FileChecker::progressChanged,
            [=](int id, const QString &name) { window->setTaskTarget(id, name); });

    connect(downloader.data(), &Downloader::rangeChanged,
            [=](int min, int max) { window->setTaskRange(min, max); });
    connect(downloader.data(), &Downloader::progressChanged,
            [=](int id, const QString &name) { window->setTaskTarget(id, name); });

    connect(window.data(), &MainWindow::taskCancelled, [=]() {
        switch (activeTask) {
        case Task::Nothing:
            log.warning("Try to cancel an empty task!");
            break;
        case Task::Checking:
            checker->cancel();
            break;
        case Task::Downloading:
            downloader->cancel();
            break;
        }
    });
}

void Ttyh::Launcher::tryBecomeOnline()
{
    log.info("Switching to online mode...");
    versions->fetchPrefixes();
    window->setLocked(true);
}

void Ttyh::Launcher::connectOnlineModeFlow()
{
    connect(window.data(), &MainWindow::onlineModeSwitched, [=](bool online) {
        if (!online) {
            log.info("Switch to offline mode");
            window->setOnline(false);
            return;
        }

        tryBecomeOnline();
    });
    connect(versions.data(), &VersionsManager::onFetchPrefixesResult, [=](bool result) {
        log.info(QString("Become %1").arg(result ? "online" : "offline"));

        if (profiles->isEmpty()) {
            auto prefixList = versions->getPrefixes().values();
            std::sort(prefixList.begin(), prefixList.end(), Prefix::less);

            for (const auto& prefix : prefixList) {
                auto profileData = ProfileData();
                profileData.version = FullVersionId(prefix.id, Prefix::latestVersionAlias);

                if (profiles->create(prefix.name, profileData) == CreateResult::Success)
                    if (!profiles->contains(settings->data.profile))
                        settings->data.profile = prefix.name;
            }
        } else if (!profiles->contains(settings->data.profile)) {
            settings->data.profile = profiles->names().first();
        }

        window->setOnline(result);
        window->setProfiles(profiles->names(), settings->data.profile);
        window->setLocked(false);

        if (!result) {
            window->showMessage(tr("Failed to switch into the online mode!"));
        } else {
            feed->requestNews();
        }
    });
    connect(feed.data(), &NewsFeed::newsReceived,
            [=](const QString &contents) { window->appendLog(contents); });
}

void Ttyh::Launcher::connectRunGameFlow()
{
    auto profileInfo = QSharedPointer<ProfileInfo>(new ProfileInfo());

    auto tryInstallVersionFiles = [=]() {
        if (!profiles->installFiles(profileInfo->name, profileInfo->data.version)) {
            window->showError(tr("Failed to install version files!"));
            window->setLocked(false);
            return false;
        }
        return true;
    };

    auto tryLogin = [=]() {
        if (!checkLoginData()) {
            window->setLocked(false);
            return;
        }
        client->login(window->getUserName(), window->getPassword());
    };

    auto handleFailedToRun = [=]() {
        window->showError(tr("Failed to prepare the game to run!"));
        window->setLocked(false);
    };

    connect(window.data(), &MainWindow::playClicked, [=]() {
        auto profileName = window->getSelectedProfile();
        if (!profiles->contains(profileName)) {
            window->showError(tr("Selected profile does not exist!"));
            return;
        }

        auto profileData = profiles->get(profileName);
        profileData.version = versions->resolve(profileData.version);

        if (profileData.version.id == Prefix::latestVersionAlias) {
            window->showError(tr("Failed to resolve the latest prefix version!"));
            return;
        }

        window->setLocked(true);
        profileInfo->name = profileName;
        profileInfo->data = profileData;

        if (!window->isOnline()) {
            if (tryInstallVersionFiles()) {
                if (!runner->run(*profileInfo, window->getUserName(), window->size()))
                    handleFailedToRun();
            }
            return;
        }

        versions->fetchVersionIndexes(profileInfo->data.version);
    });
    connect(versions.data(), &VersionsManager::onFetchVersionIndexesResult, [=](bool result) {
        QList<FileInfo> files;
        if (!versions->fillVersionFiles(profileInfo->data.version, files)) {
            window->showError(tr("Failed to get the version indexes!"));
            window->setLocked(false);
            return;
        }

        showTask(Task::Checking);
        checker->start(files);
    });
    connect(checker.data(), &FileChecker::finished, [=](bool cancel, const QList<FileInfo> &files) {
        hideTask();

        if (cancel) {
            window->setLocked(false);
            return;
        }

        if (files.isEmpty()) {
            if (tryInstallVersionFiles())
                tryLogin();

            return;
        }

        quint64 size = 0;
        for (const auto& fileInfo : files)
            size += fileInfo.size;

        if (!window->askForDownloads(files.count(), size)) {
            window->setLocked(false);
            return;
        }

        showTask(Task::Downloading);
        downloader->start(files);
    });
    connect(downloader.data(), &Downloader::finished, [=](bool cancel, bool result) {
        hideTask();

        if (cancel) {
            window->setLocked(false);
            return;
        }

        if (!result) {
            window->showError(tr("Failed to download version files!"));
            window->setLocked(false);
            return;
        }

        if (tryInstallVersionFiles())
            tryLogin();
    });
    connect(client.data(), &TtyhClient::logged,
            [=](RequestResult result, const QString &accessToken, const QString &clientToken) {
                if (result != RequestResult::Success) {
                    window->showError(getRequestResultMessage(result));
                    window->setLocked(false);
                    return;
                }

                auto userName = window->getUserName();
                auto windowSize = window->size();
                if (!runner->run(*profileInfo, userName, accessToken, clientToken, windowSize))
                    handleFailedToRun();
            });

    connect(runner.data(), &ProfileRunner::startHandled, [=](bool result) {
        if (!result) {
            window->setLocked(false);
            window->showError(tr("Failed to start the game!"));
            return;
        }

        if (window->isHideOnRun())
            window->hide();
    });
    connect(runner.data(), &ProfileRunner::finished, [=](bool result) {
        window->setLocked(false);

        if (window->isHideOnRun())
            window->show();

        if (!result) {
            window->showError(tr("Game finished with a error!"));
        }
    });
}

void Ttyh::Launcher::connectProfileActions()
{
    connect(window.data(), &MainWindow::profileEditClicked, [=]() {
        auto profileName = window->getSelectedProfile();
        if (!profiles->contains(profileName)) {
            window->showError(tr("Selected profile does not exist!"));
            return;
        }

        auto profileData = profiles->get(profileName);
        ProfileDialog dlg(window.data(), profileName, profileData, versions->getPrefixes());
        connect(&dlg, &ProfileDialog::saveClicked,
                [=, &dlg](const QString &newName, const ProfileData &newData) {
                    if (profileName != newName) {
                        auto renameResult = profiles->rename(profileName, newName);
                        if (renameResult != RenameResult::Success) {
                            dlg.showRenameError(renameResult);
                            return;
                        }

                        window->setProfiles(profiles->names(), newName);
                    }

                    auto updated = profiles->update(newName, newData);
                    if (!updated)
                        dlg.showError(tr("Failed to save profile data!"));

                    dlg.close();
                });
        dlg.exec();
    });

    connect(window.data(), &MainWindow::profileCreateClicked, [=]() {
        ProfileDialog dlg(window.data(), tr("New Profile"), ProfileData(), versions->getPrefixes());
        connect(&dlg, &ProfileDialog::saveClicked,
                [=, &dlg](const QString &name, const ProfileData &data) {
                    auto createResult = profiles->create(name, data);
                    if (createResult != CreateResult::Success) {
                        dlg.showCreateError(createResult);
                        return;
                    }

                    window->setProfiles(profiles->names(), name);
                    dlg.close();
                });
        dlg.exec();
    });

    connect(window.data(), &MainWindow::profileRemoveClicked, [=]() {
        auto profileName = window->getSelectedProfile();
        if (!profiles->contains(profileName)) {
            window->showError(tr("Selected profile does not exist!"));
            return;
        }

        if (!window->askForProfileDeletion(profileName))
            return;

        if (profiles->remove(profileName)) {
            auto profilesNames = profiles->names();
            auto select = profilesNames.isEmpty() ? "" : profilesNames.first();
            window->setProfiles(profilesNames, select);
        } else {
            window->showError(tr("Failed to remove the selected profile!"));
        }
    });
}

void Ttyh::Launcher::connectSkinUpload()
{
    connect(window.data(), &MainWindow::uploadSkinClicked, [=]() {
        if (!checkLoginData())
            return;

        SkinDialog d(window.data());

        connect(&d, &SkinDialog::uploadClicked, [=, &d](const QString &path, bool slim) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                d.showError(tr("Failed to open the skin file!"));
                return;
            }

            client->uploadSkin(window->getUserName(), window->getPassword(), file.readAll(), slim);
        });

        auto conn = connect(client.data(), &TtyhClient::skinUploaded, [=, &d](RequestResult res) {
            if (res == RequestResult::Success) {
                d.showSuccessAndClose();
                return;
            }

            d.showError(getRequestResultMessage(res));
        });

        d.exec();
        disconnect(conn);
    });
}

void Ttyh::Launcher::start()
{
    log.info("Starting...");
    loadWindowState();
    tryBecomeOnline();

    if (settings->data.windowMaximized) {
        window->showMaximized();
    } else {
        window->show();
    }
}

void Ttyh::Launcher::showTask(Task task)
{
    if (activeTask != Task::Nothing) {
        auto strOldTask = taskToString(activeTask);
        auto strNewTask = taskToString(task);
        log.error(QString("Replace the task '%1' by '%2'").arg(strOldTask, strNewTask));
    }

    activeTask = task;
    window->showTask(getTaskTitle(task));
}

void Ttyh::Launcher::hideTask()
{
    activeTask = Task::Nothing;
    window->hideTask();
}

QString Ttyh::Launcher::taskToString(Task task)
{
    switch (task) {
    case Task::Nothing:
        return "Task::Nothing";
    case Task::Checking:
        return "Task::Checking";
    case Task::Downloading:
        return "Task::Downloading";
    default:
        return "";
    }
}

QString Ttyh::Launcher::getTaskTitle(Task task)
{
    switch (task) {
    case Task::Nothing:
        return "Task::Nothing";
    case Task::Checking:
        return tr("Checking files");
    case Task::Downloading:
        return tr("Downloading files");
    default:
        return "";
    }
}

QString Ttyh::Launcher::getRequestResultMessage(RequestResult result)
{
    switch (result) {
    case RequestResult::Success:
        return "RequestResult::Success";
    case RequestResult::ConnectionError:
        return tr("Failed to connect to the server");
    case RequestResult::LoginError:
        return tr("Incorrect login or password");
    case RequestResult::RequestError:
        return tr("Bad request data");
    case RequestResult::ReplyError:
        return tr("Incorrect server reply");
    default:
        return "";
    }
}

void Ttyh::Launcher::loadWindowState()
{
    log.info("Set the window state by the settings");

    window->setProfiles(profiles->names(), settings->data.profile);
    window->setUserName(settings->data.username);
    window->setPassword(settings->data.password);

    window->setSavePassword(settings->data.savePassword);
    window->setHideOnRun(settings->data.hideWindowOnRun);

    window->resize(settings->data.windowSize);
}

void Ttyh::Launcher::saveWindowState()
{
    log.info("Set settings state by the window");
    settings->data.profile = window->getSelectedProfile();

    settings->data.username = window->getUserName();
    settings->data.savePassword = window->isSavePassword();

    if (settings->data.savePassword) {
        settings->data.password = window->getPassword();
    } else {
        settings->data.password = "";
    }

    settings->data.hideWindowOnRun = window->isHideOnRun();
    settings->data.windowMaximized = window->isMaximized();
}

bool Ttyh::Launcher::checkLoginData()
{
    if (window->getUserName().isEmpty() || window->getPassword().isEmpty()) {
        window->showError(tr("Need to specify the login and password"));
        return false;
    }

    return true;
}
