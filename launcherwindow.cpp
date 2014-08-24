#include "launcherwindow.h"
#include "ui_launcherwindow.h"

#include "settingsdialog.h"
#include "passworddialog.h"
#include "skinuploaddialog.h"
#include "updatedialog.h"
#include "feedbackdialog.h"
#include "aboutdialog.h"

#include "settings.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QTimer>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QByteArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include <QJsonObject>
#include <QFileInfo>

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#include <quazip/zip.h>
#include <quazip/ioapi.h>

LauncherWindow::LauncherWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LauncherWindow)
{
    // Setup form from ui-file
    ui->setupUi(this);

    // Setup settings
    settings = Settings::instance();

    // Make news menuitems like radiobuttons (it's impossible from qt-designer)
    QActionGroup *newsGroup = new QActionGroup(this);
    newsGroup->addAction(ui->ttyhNews);
    newsGroup->addAction(ui->officialNews);

    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(ui->webView, SIGNAL(linkClicked(const QUrl&)), SLOT(linkClicked(const QUrl&)));

    // News Menu connections
    connect(ui->ttyhNews, SIGNAL(changed()), SLOT(loadTtyh()));
    connect(ui->officialNews, SIGNAL(changed()), SLOT(loadOfficial()));

    // Options Menu connections
    connect(ui->runSettings, SIGNAL(triggered()), SLOT(showSettingsDialog()));

    // Additional Menu connections
    connect(ui->changePassword, SIGNAL(triggered()), SLOT(showChangePasswordDialog()));
    connect(ui->changeSkin, SIGNAL(triggered()), SLOT(showSkinLoadDialog()));
    connect(ui->updateManager, SIGNAL(triggered()), SLOT(showUpdateManagerDialog()));

    // Help Menu connections
    connect(ui->bugReport, SIGNAL(triggered()), SLOT(showFeedBackDialog()));
    connect(ui->aboutLauncher, SIGNAL(triggered()), SLOT(showAboutDialog()));

    // Setup login field
    ui->nickEdit->setText(settings->loadLogin());
    // Save login when changed
    connect(ui->nickEdit, SIGNAL(textChanged(QString)), settings, SLOT(saveLogin(QString)));

    // Setup password field
    ui->savePassword->setChecked(settings->loadPassStore());
    if (ui->savePassword->isChecked())
        ui->passEdit->setText(settings->loadPassword());
    // Password are saved on login or exit if savePassword is checked
    connect(ui->savePassword, SIGNAL(clicked(bool)), settings, SLOT(savePassStore(bool)));

    // Setup client combobox
    ui->clientCombo->addItems(settings->getClientsNames());
    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
    connect(ui->clientCombo, SIGNAL(activated(int)), settings, SLOT(saveActiveClientId(int)));

    // Setup news set
    ui->ttyhNews->setChecked(true);
    emit ui->ttyhNews->changed();

    // Setup window parameters
    QRect geometry = settings->loadWindowGeometry();
    // Centering window, if loaded default values
    if (geometry.x() < 0)
        this->move(QApplication::desktop()->screen()->rect().center() - this->rect().center());
    else
        this->setGeometry(geometry);

    // Restore maximized state
    if (settings->loadMaximizedState()) this->showMaximized();

    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(playButtonClicked()));

}

void LauncherWindow::closeEvent (QCloseEvent* event) { storeParameters(); }

// Run this method on close window and run game
void LauncherWindow::storeParameters() {
    settings->saveWindowGeometry(this->geometry());
    settings->saveMaximizedState(this->isMaximized());

    if (ui->savePassword->isChecked())
        settings->savePassword(ui->passEdit->text());
    // Security issue
    else
        settings->savePassword("");
}

// Show dialog slots
void LauncherWindow::showSettingsDialog() {
    SettingsDialog* d = new SettingsDialog(this);
    d->exec();
    delete d;

    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
}

void LauncherWindow::showChangePasswordDialog() {
    PasswordDialog* d = new PasswordDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showSkinLoadDialog() {
    SkinUploadDialog* d = new SkinUploadDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showUpdateManagerDialog() {
    UpdateDialog* d = new UpdateDialog(this);
    d->exec();
    delete d;

    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
}

void LauncherWindow::showFeedBackDialog() {
    FeedbackDialog* d = new FeedbackDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showAboutDialog() {
    AboutDialog* d = new AboutDialog(this);
    d->exec();
    delete d;
}

// Load webpage slots
void LauncherWindow::loadTtyh() {loadPage(QUrl("http://ttyh.ru"));}
void LauncherWindow::loadOfficial() {loadPage(QUrl("http://mcupdate.tumblr.com/"));}

// Open external browser slot
void LauncherWindow::linkClicked(const QUrl& url) {if (!QDesktopServices::openUrl(url)) qDebug() << "Failed to open system browser!";}

// Load webpage method
void LauncherWindow::loadPage(const QUrl& url) {
    // Show "loading..." during webpage load
    // FIXME: this is not working
    ui->webView->load(QUrl("qrc:/resources/loading.html"));
    ui->webView->load(url);

    // Setup timeout timer
    QTimer* ptimer = new QTimer(ui->webView);
    connect(ptimer, SIGNAL(timeout()), SLOT(loadPageTimeout()));                // Show error page on timeout
    connect(ui->webView, SIGNAL(loadFinished(bool)), SLOT(pageLoaded(bool)));   // Show error page on loading fail
    connect(ui->webView, SIGNAL(loadFinished(bool)), ptimer, SLOT(stop()));
    ptimer->start(10000); // Timeout value
}

// Slots used by LoadPage method
void LauncherWindow::loadPageTimeout() {ui->webView->load(QUrl("qrc:/resources/error.html"));}
void LauncherWindow::pageLoaded(bool loaded) {if (!loaded) ui->webView->load(QUrl("qrc:/resources/error.html"));}

void LauncherWindow::playButtonClicked() {

    ui->playButton->setEnabled(false);

    if (!ui->playOffline->isChecked()) {

        QNetworkAccessManager* manager = new QNetworkAccessManager(this);

        // Make JSON login request, see: http://wiki.vg/Authentication
        QNetworkRequest loginRequest;
        QJsonDocument jsonRequest;
        QJsonObject reqtData, reqAgent, reqPlatform;

        reqAgent["name"] = "Minecraft";
        reqAgent["version"] = 1;

        reqPlatform["os"] = settings->getOsName();
        reqPlatform["version"] = settings->getOsVersion();
        reqPlatform["word"] = settings->getWordSize();

        reqtData["agent"] = reqAgent;
        reqtData["platform"] = reqPlatform;
        reqtData["username"] = ui->nickEdit->text();
        reqtData["password"] = ui->passEdit->text();
        reqtData["clientToken"] = settings->makeMinecraftUuid();

        jsonRequest.setObject(reqtData);

        QByteArray postdata;
        postdata.append(jsonRequest.toJson());

        loginRequest.setUrl(Settings::authUrl);
        loginRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        loginRequest.setHeader(QNetworkRequest::ContentLengthHeader, postdata.size());

        QNetworkReply *loginReply = manager->post(loginRequest, postdata);
        QEventLoop loop;
        connect(loginReply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        // Check for connection error
        if (loginReply->error() == QNetworkReply::NoError) {

            QByteArray rawLoginReply = loginReply->readAll();
            QJsonParseError error;
            QJsonDocument jsonLoginReply = QJsonDocument::fromJson(rawLoginReply, &error);

            // Check for incorrect JSON
            if (error.error == QJsonParseError::NoError) {

                QJsonObject replyData = jsonLoginReply.object();

                // Check for error in server answer
                if (replyData["error"].toString() != "") {
                    // Error in answer handler
                    QString cause = replyData["cause"].toString();
                    if (cause != "") cause = "\n\n Причина: " + cause;
                    QMessageBox::critical(this, "У нас проблема :(",
                                          replyData["errorMessage"].toString()
                                          + cause);
                } else {

                    // Correct login, run game in online-mode
                    QString uuid = replyData["clientToken"].toString();
                    QString acessToken = replyData["accessToken"].toString();
                    QString gameVersion = settings->loadClientVersion();

                    // Switch from "latest" to real version
                    bool run = true;

                    if (gameVersion == "latest") {
                        QNetworkRequest versionRequest;
                        versionRequest.setUrl(QUrl("https://s3.amazonaws.com/Minecraft.Download/versions/versions.json"));

                        QNetworkReply *versionReply = manager->get(versionRequest);
                        QEventLoop loop;
                        connect(versionReply, SIGNAL(finished()), &loop, SLOT(quit()));
                        loop.exec();

                        if (versionReply->error() == QNetworkReply::NoError) {

                            QByteArray rawVersionReply = versionReply->readAll();
                            QJsonParseError error;
                            QJsonDocument jsonVersionReply = QJsonDocument::fromJson(rawVersionReply, &error);

                            // Check for incorrect JSON
                            if (error.error == QJsonParseError::NoError) {

                                QJsonObject latest = jsonVersionReply.object()["latest"].toObject();
                                gameVersion = latest["release"].toString();
                                if (gameVersion == "") run = false;

                            } else {
                                QMessageBox::critical(this, "У нас проблема :(",
                                                      "Не удалось понять что же нужно запустить...\n"
                                                      + error.errorString() + " в поз. "  + QString::number(error.offset));
                                run = false;
                            }

                        } else {
                            QMessageBox::critical(this, "У нас проблема :(",
                                                  "Не удалось определить версию для запуска!\n"
                                                  + versionReply->errorString());
                            run = false;
                        }

                    }

                    if (run) runGame(uuid, acessToken, gameVersion);

                }

            } else {
                // JSON parse error
                QMessageBox::critical(this, "У нас проблема :(",
                                      "Упс... При попытке логина сервер овтетил ерунду...\n\n" +
                                      error.errorString() +
                                      " в позиции " + QString::number(error.offset));
            }


        } else {
            // Connection error
            if (loginReply->error() == QNetworkReply::AuthenticationRequiredError) {
                QMessageBox::critical(this, "У нас проблема!",
                                      "Ошибка авторизации.\nНеправильный логин или пароль\n\t...или они оба неправильные :(");
            } else {
                QMessageBox::critical(this, "У нас проблема :(",
                                      "Упс... Вот ведь незадача...\n\n" +
                                      loginReply->errorString());
            }

        }

        delete manager;

    } else {

        // Run game in offline mode
        QString uuid = "HARD";
        QString acessToken = "CORE";
        QString gameVersion = settings->loadClientVersion();

        bool run = true;
        if (gameVersion == "latest") {

            // Great old date for comparsion!
            // releaseDate used for store latest founded release date and write gameVersion if this happened
            QDateTime releaseDate = QDateTime::fromString("1991-05-18T13:15:00+07:00", Qt::ISODate);

            QDir verDir = QDir(settings->getVersionsDir());
            QStringList verList = verDir.entryList();

            for (QStringList::iterator nameit = verList.begin(), end = verList.end(); nameit != end; ++nameit) {
                QString currentVersion = (*nameit);
                QFile* versionFile = new QFile(settings->getVersionsDir()
                                               + "/" + currentVersion
                                               + "/" + currentVersion + ".json");
                if (versionFile->open(QIODevice::ReadOnly)) {

                    QJsonParseError error;
                    QJsonDocument versionJson =  QJsonDocument::fromJson(versionFile->readAll(), &error);
                    if (error.error == QJsonParseError::NoError) {

                        QString currentTimeStr = versionJson.object()["releaseTime"].toString();
                        QDateTime curRelTime = QDateTime::fromString(currentTimeStr, Qt::ISODate);

                        if (curRelTime.isValid() && (curRelTime > releaseDate)) {
                            releaseDate = curRelTime;
                            gameVersion = currentVersion;
                        }

                    }
                    versionFile->close();
                }
                delete versionFile;
            }

            if (gameVersion == "latest") {
                QMessageBox::critical(this, "У нас проблема :(",
                                      "Похоже, что не установлено\nни одной версии игры.\n\tЭто печально!");
                run = false;
            }
        }

        if (run) runGame(uuid, acessToken, gameVersion);

    }

    ui->playButton->setEnabled(true);
}

void LauncherWindow::runGame(QString uuid, QString acessToken, QString gameVersion) {

    QString java, libpath, classpath,
            mainClass, minecraftArguments;

    // Setup java binary
    if (settings->loadClientJavaState()) {
        java = settings->loadClientJava();
    } else {
        java = "java";
    }

    // Prepare library path
    libpath = settings->getNativesDir();
    recursiveDelete(libpath);

    QDir libsDir = QDir(libpath);
    libsDir.mkpath(libpath);

    if (!libsDir.exists()) {
        QMessageBox::critical(this, "У нас проблема :(",
                              "Не удалось подготовить LIBRARY_PATH. Sorry!");
        return;
    }

    // Setup classpath and extract natives!
    QFile* versionFile = new QFile(settings->getVersionsDir() + "/" + gameVersion + "/" + gameVersion + ".json");

    if (versionFile->open(QIODevice::ReadOnly)) {

        QJsonParseError error;
        QJsonDocument versionJson = QJsonDocument::fromJson(versionFile->readAll(), &error);

        if (error.error == QJsonParseError::NoError) {

            QJsonArray libraries = versionJson.object()["libraries"].toArray();
            for (QJsonArray::iterator libit = libraries.begin(), end = libraries.end(); libit != end; ++libit) {

                QJsonObject library = (*libit).toObject();

                QStringList entry = library["name"].toString().split(':');

                // <package>:<name>:<version> to <package>/<name>/<version>/<name>-<version> and chahge <backage> format from a.b.c to a/b/c
                QString libSuffix = entry.at(0);                  // package
                libSuffix.replace('.', '/');                      // package format
                libSuffix += "/" + entry.at(1)                    // + name
                        + "/" + entry.at(2)                       // + version
                        + "/" + entry.at(1) + "-" + entry.at(2);  // + name-version

                if (library["natives"].isNull()) {
                    libSuffix += ".jar:";
                    classpath += settings->getLibsDir() + "/" + libSuffix;

                } else {
                    if (!library["natives"].toObject()[settings->getOsName()].isNull()) {
                        libSuffix += "-natives-" + settings->getOsName() + ".jar";
                        unzipAllFiles(settings->getLibsDir() + "/" + libSuffix,
                                      settings->getNativesDir());
                    }
                }


            }

            // Add game jar to classpath
            classpath += settings->getVersionsDir() + "/" + gameVersion + "/" + gameVersion + ".jar";

            // Setup mainClass
            if (versionJson.object()["mainClass"].isNull()) {
                QMessageBox::critical(this, "У нас проблема :(",
                                      "Вот беда. В конфигурационном файле не указан mainClass.");
                return;
            } else {
                mainClass = versionJson.object()["mainClass"].toString();
            }

            // Read assets index
            QString assetsIndex;
            if (versionJson.object()["assets"].isNull()) {
                QMessageBox::critical(this, "У нас проблема !!!",
                                      "Аааа! В конфигурационном файле не указаны ассеты!");
                return;
            } else {
                assetsIndex = versionJson.object()["assets"].toString();
            }


            // Setup aruments
            if (versionJson.object()["minecraftArguments"].isNull()) {
                QMessageBox::critical(this, "У нас проблема :(",
                                      "В конфигурационном файле не указаны аргументы запуска.");
                return;
            } else {
                minecraftArguments = versionJson.object()["minecraftArguments"].toString();
            }
            // QString uuid, QString acessToken, QString gameVersion
            minecraftArguments.replace("${auth_player_name}",  settings->loadLogin());
            minecraftArguments.replace("${version_name}",      gameVersion);
            minecraftArguments.replace("${game_directory}",    settings->getClientDir());
            minecraftArguments.replace("${assets_root}",       settings->getAssetsDir());
            minecraftArguments.replace("${assets_index_name}", assetsIndex);
            minecraftArguments.replace("${auth_uuid}",         uuid);
            minecraftArguments.replace("${auth_access_token}", acessToken);

            // What it means?
            minecraftArguments.replace("${user_properties}",   "{}");
            //minecraftArguments.replace("${user_type}",         "${user_type}";

            // RUN-RUN-RUN!
            QStringList args;
            args << "-Djava.library.path=" + libpath
                 << "-cp" << classpath
                 << mainClass
                 << minecraftArguments.split(" ");

            QProcess::execute(java, args);

        } else {
            QMessageBox::critical(this, "У нас проблема :(",
                                  "Зело непонятный конфигурационный файл...\n"
                                  + error.errorString() + "  поз. " + QString::number(error.offset));
            delete versionFile;
            return;
        }

        versionFile->close();
    } else {
        QMessageBox::critical(this, "У нас проблема :(",
                              "Не удалось открыть конфигурационный файл!");
        delete versionFile;
        return;
    }

    delete versionFile;

}

void LauncherWindow::recursiveDelete(QString filePath) {
    QFileInfo fileInfo = QFileInfo(filePath);

    if (fileInfo.isDir()) {
        QStringList lstFiles = QDir(filePath).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        if (!lstFiles.isEmpty()) {
            foreach (QString entry, lstFiles)
                recursiveDelete(filePath + "/" + entry);
        }
        QDir(filePath).rmdir(fileInfo.absoluteFilePath());

    } else if (fileInfo.exists()) {
        QFile::remove(filePath);
    }
}

void LauncherWindow::unzipAllFiles(QString zipFilePath, QString extractionPath) {

    // Open ZIP
    QuaZip zip(zipFilePath);
    if (zip.open(QuaZip::mdUnzip)) {

        QuaZipFile zipFile(&zip);

        for (bool f=zip.goToFirstFile(); f; f=zip.goToNextFile()) {
            zipFile.open(QIODevice::ReadOnly);

            QFile* realFile = new QFile(extractionPath + "/" + zip.getCurrentFileName());

            QFileInfo rfInfo = QFileInfo(*realFile);

            QDir rfDir = rfInfo.absoluteDir();
            rfDir.mkpath(rfDir.absolutePath());

            if (!rfInfo.isDir()) {
                if (realFile->open(QIODevice::WriteOnly)) {
                    realFile->write(zipFile.readAll());
                    realFile->close();
                } else {
                    // FIXME
                    qDebug() << realFile->errorString();
                }
            }

            delete realFile;
            zipFile.close();
        }
        zip.close();
    }

}



LauncherWindow::~LauncherWindow()
{
    delete ui;
    delete newsGroup;
}
