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

LauncherWindow::LauncherWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LauncherWindow)
{
    // Setup form from ui-file
    ui->setupUi(this);

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

    // Connect to settings
    Settings* settings = Settings::instance();

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

    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(startGame()));

}

void LauncherWindow::closeEvent (QCloseEvent* event) { storeParameters(); }

// Run this method on close window and run game
void LauncherWindow::storeParameters() {
    Settings* settings = Settings::instance();
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

    Settings* settings = Settings::instance();
    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
}

void LauncherWindow::showChangePasswordDialog() {
    PasswordDialog* d = new PasswordDialog(this);
    d->exec();
}

void LauncherWindow::showSkinLoadDialog() {
    SkinUploadDialog* d = new SkinUploadDialog(this);
    d->exec();
}

void LauncherWindow::showUpdateManagerDialog() {
    UpdateDialog* d = new UpdateDialog(this);
    d->exec();

    Settings* settings = Settings::instance();
    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
}

void LauncherWindow::showFeedBackDialog() {
    FeedbackDialog* d = new FeedbackDialog(this);
    d->exec();
}

void LauncherWindow::showAboutDialog() {
    AboutDialog* d = new AboutDialog(this);
    d->exec();
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

void LauncherWindow::startGame() {

    ui->playButton->setEnabled(false);

   QString loginURL ="http://master.ttyh.ru/index.php?act=login";

   QNetworkAccessManager* manager = new QNetworkAccessManager(this);

   // Make JSON login request, see: http://wiki.vg/Authentication
   QJsonDocument data;
   QJsonObject login, agent;
   QNetworkRequest request;

   agent["name"] = "Minecraft";
   agent["version"] = 1;

   login["agent"] = agent;
   login["username"] = ui->nickEdit->text();
   login["password"] = ui->passEdit->text();
   login["clientToken"] = "tok-tok-en";

   data.setObject(login);

   QByteArray postdata;
   postdata.append(data.toJson());

   request.setUrl(loginURL);
   request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
   request.setHeader(QNetworkRequest::ContentLengthHeader, postdata.size());

   QNetworkReply *reply = manager->post(request, postdata);
   QEventLoop loop;
   connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
   loop.exec();

   // Check for connection error
   if (reply->error() == QNetworkReply::NoError) {

       QByteArray rawResponce = reply->readAll();
       qDebug() << rawResponce;
       QJsonParseError error;
       QJsonDocument json = QJsonDocument::fromJson(rawResponce, &error);

       // Check for incorrect JSON
       if (error.error == QJsonParseError::NoError) {

           QJsonObject responce = json.object();

           // Check for error in server answer
           if (responce["error"].toString() != "") {
               // Error in answer handler
               QString cause = responce["cause"].toString();
               if (cause != "") cause = "\n\n Причина: " + cause;
               QMessageBox::critical(this, "У нас проблема :(",
                                     responce["errorMessage"].toString()
                                     + cause);
           } else {
               // Correct login
               QMessageBox::information(this, "Responce", "aid = " + responce["accessToken"].toString() +
                       "\n" + "cid = " + responce["clientToken"].toString());
           }

       } else {
           // JSON parse error
           QMessageBox::critical(this, "У нас проблема :(",
                                 "Упс... Сервер овтетил ерунду...\n\n" +
                                 error.errorString() +
                                 " в позиции " + QString::number(error.offset));
       }


   } else {
       // Connection error
       if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
           QMessageBox::critical(this, "У нас проблема :(",
                                 "Ошибка авторизации.\nНеправильный логин или пароль.");
       } else {
           QMessageBox::critical(this, "У нас проблема :(",
                                 "Упс... Вот ведь незадача...\n\n" +
                                 reply->errorString());
       }

   }



   ui->playButton->setEnabled(true);
}

LauncherWindow::~LauncherWindow()
{
    delete ui;
    delete newsGroup;
}
