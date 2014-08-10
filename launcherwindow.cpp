#include "launcherwindow.h"
#include "ui_launcherwindow.h"

#include "settingsdialog.h"
#include "passworddialog.h"
#include "skinuploaddialog.h"
#include "updatedialog.h"
#include "feedbackdialog.h"
#include "aboutdialog.h"

#include <QTimer>
#include <QDesktopServices>

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
    ui->ttyhNews->setChecked(true); // FIXME: need to restore news state from settings
    loadPage(QUrl("http://ttyh.ru"));

    // Options Menu connections
    connect(ui->runSettings, SIGNAL(triggered()), SLOT(showSettingsDialog()));

    // Additional Menu connections
    connect(ui->changePassword, SIGNAL(triggered()), SLOT(showChangePasswordDialog()));
    connect(ui->changeSkin, SIGNAL(triggered()), SLOT(showSkinLoadDialog()));
    connect(ui->updateManager, SIGNAL(triggered()), SLOT(showUpdateManagerDialog()));

    // Help Menu connections
    connect(ui->bugReport, SIGNAL(triggered()), SLOT(showFeedBackDialog()));
    connect(ui->aboutLauncher, SIGNAL(triggered()), SLOT(showAboutDialog()));
}

// Show dialog slots
void LauncherWindow::showSettingsDialog() { SettingsDialog* d = new SettingsDialog(this); d->show(); }
void LauncherWindow::showChangePasswordDialog() { PasswordDialog* d = new PasswordDialog(this); d->show(); }
void LauncherWindow::showSkinLoadDialog() { SkinUploadDialog* d = new SkinUploadDialog(this); d->show(); }
void LauncherWindow::showUpdateManagerDialog() { UpdateDialog* d = new UpdateDialog(this); d->show(); }
void LauncherWindow::showFeedBackDialog() { FeedbackDialog* d = new FeedbackDialog(this); d->show(); }
void LauncherWindow::showAboutDialog() { AboutDialog* d = new AboutDialog(this); d->show(); }

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

LauncherWindow::~LauncherWindow()
{
    delete ui;
    delete newsGroup;
}
