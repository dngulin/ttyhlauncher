#include "launcherwindow.h"
#include "ui_launcherwindow.h"

#include "settingsdialog.h"
#include "skinuploaddialog.h"
#include "updatedialog.h"
#include "feedbackdialog.h"
#include "aboutdialog.h"

#include "clonedialog.h"
#include "fetchdialog.h"
#include "checkoutdialog.h"
#include "exportdialog.h"

#include "settings.h"
#include "util.h"
#include "jsonparser.h"

#include <QtGui>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QShortcut>

#include <QDebug>

LauncherWindow::LauncherWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LauncherWindow)
{
    // Setup form from ui-file
    ui->setupUi(this);

    // Setup settings and logger
    settings = Settings::instance();
    logger = Logger::logger();

    ui->logDisplay->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    appendToLog( tr("Welcome to the ttyhlauncher.\n") );

    QFile logoFile(":/resources/logo.txt");
    if (logoFile.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream logoStream(&logoFile);
        while (!logoStream.atEnd())
        {
            appendToLog(logoStream.readLine() + QString("\n"));
        }
        logoFile.close();
    }
    else
    {
        logger->append(this->objectName(), tr("Can't open logo resource.\n"));
    }

    connect(logger, SIGNAL(textAppended(QString)), this, SLOT(appendToLog(QString)));

    // Options Menu connections
    connect(ui->runSettings, SIGNAL(triggered()), SLOT(showSettingsDialog()));

    // Additional Menu connections
    connect(ui->changeSkin, SIGNAL(triggered()), SLOT(showSkinLoadDialog()));
    connect(ui->updateManager, SIGNAL(triggered()), SLOT(showUpdateManagerDialog()));

    // Help Menu connections
    connect(ui->bugReport, SIGNAL(triggered()), SLOT(showFeedBackDialog()));
    connect(ui->aboutLauncher, SIGNAL(triggered()), SLOT(showAboutDialog()));

    // Client builder menu visibility and connect entries
    ui->builderMenu->menuAction()->setVisible(false);
    new QShortcut(QKeySequence(Qt::ALT + Qt::Key_B), this, SLOT(switchBuilderMenuVisibility()));

    connect(ui->doClone, SIGNAL(triggered()), this, SLOT(showCloneDialog()));
    connect(ui->doFetch, SIGNAL(triggered()), this, SLOT(showFetchDialog()));
    connect(ui->doCheckout, SIGNAL(triggered()), this, SLOT(showCheckoutDialog()));
    connect(ui->doExport, SIGNAL(triggered()), this, SLOT(showExportDialog()));

    // Setup offlineMode entry
    ui->playOffline->setChecked(settings->loadOfflineModeState());
    this->offlineModeChanged();
    connect(ui->playOffline, SIGNAL(triggered()), SLOT(offlineModeChanged()));

    // Setup hidewindow entry
    ui->hideLauncher->setChecked(settings->loadHideWindowModeState());
    this->hideWindowModeChanged();
    connect(ui->hideLauncher, SIGNAL(triggered()), SLOT(hideWindowModeChanged()));

    // Setup login field
    ui->nickEdit->setText(settings->loadLogin());
    // Save login when changed
    connect(ui->nickEdit, SIGNAL(textChanged(QString)), settings, SLOT(saveLogin(QString)));

    // Setup password field
    ui->savePassword->setChecked(settings->loadPassStoreState());
    if (ui->savePassword->isChecked())
        ui->passEdit->setText(settings->loadPassword());
    // Password are saved on login or exit if savePassword is checked
    connect(ui->savePassword, SIGNAL(clicked(bool)), settings, SLOT(savePassStoreState(bool)));

    // Setup client combobox
    ui->clientCombo->addItems(settings->getClientsNames());
    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
    connect(ui->clientCombo, SIGNAL(activated(int)), settings, SLOT(saveActiveClientId(int)));

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

    logger->append(this->objectName(), "Launcher window opened\n");

    if (ui->clientCombo->count() == 0) {
        this->show(); // hack to show window before error message
        ui->playButton->setEnabled(false);
        QMessageBox::critical(this, "Беда-беда!",  "Не удалось получить список клиентов\nМы все умрём.\n");
    }

}

void LauncherWindow::closeEvent (QCloseEvent* event) {
    logger->append(this->objectName(), "Launcher window closed\n");
    emit windowClosed();
    storeParameters();
    event->accept();
}

void LauncherWindow::keyPressEvent(QKeyEvent* pe) {
    if(pe->key() == Qt::Key_Return) playButtonClicked();
    pe->accept();
}

void LauncherWindow::appendToLog(const QString &text)
{
    QRegularExpression urlRegEx("((?:https?)://\\S+)");

    QRegularExpressionMatch urlMatch = urlRegEx.match(text);

    ui->logDisplay->moveCursor(QTextCursor::End);

    if (urlMatch.hasMatch())
    {
        int matches = urlMatch.lastCapturedIndex();

        int lastIndex = 0;
        for (int i = 1; i <= matches; i++)
        {
            int from = urlMatch.capturedStart(i);
            int to   = urlMatch.capturedEnd(i);

            // Add before link part
            if (lastIndex < from)
            {
                QString sub = text.mid(lastIndex, from - lastIndex);
                ui->logDisplay->insertHtml(escapeString(sub));
            }

            // Add link
            QString url = text.mid(from, to - from);
            QString link = QString(" <a href='") + url + QString("'>"  )
                                                 + url + QString("</a>");

            ui->logDisplay->insertHtml(link);
            lastIndex = to;

            // Append last after link part
            if (i == matches && lastIndex < text.length())
            {
                QString sub = text.mid(lastIndex, text.length() - lastIndex);
                ui->logDisplay->insertHtml(escapeString(sub));
            }
        }
    }
    else
    {
        ui->logDisplay->insertHtml(escapeString(text));
    }
}

QString LauncherWindow::escapeString(const QString &string)
{
    return string
            .toHtmlEscaped()
            .replace("\n", "<br>\n")
            .replace(" ", "&nbsp;");
}

void LauncherWindow::switchBuilderMenuVisibility() {
    if (ui->builderMenu->menuAction()->isVisible()) {
        ui->builderMenu->menuAction()->setVisible(false);
    } else {
        ui->builderMenu->menuAction()->setVisible(true);
    }
}

void LauncherWindow::showCloneDialog() {
    CloneDialog* d = new CloneDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showFetchDialog() {
    FetchDialog* d = new FetchDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showCheckoutDialog() {
    CheckoutDialog* d = new CheckoutDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showExportDialog() {
    ExportDialog* d = new ExportDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showError(const QString &title, const QString &message)
{
    QMessageBox::critical(this, title, message);
}

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

void LauncherWindow::showSkinLoadDialog() {
    SkinUploadDialog* d = new SkinUploadDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showUpdateManagerDialog() {
    showUpdateDialog("Для проверки наличия обновлений выберите нужный клиент и нажмите кнопку \"Проверить\"");
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

void LauncherWindow::offlineModeChanged() {
    settings->saveOfflineModeState(ui->playOffline->isChecked());
    settings->loadOfflineModeState() ? ui->playButton->setText("Играть (оффлайн)") :
                                       ui->playButton->setText("Играть");
}

void LauncherWindow::hideWindowModeChanged() {
    settings->saveHideWindowModeState(ui->hideLauncher->isChecked());
}

void LauncherWindow::freezeInterface() {

    ui->runPanel->setEnabled(false);
    ui->menuBar->setEnabled(false);
}

void LauncherWindow::unfreezeInterface() {

    ui->runPanel->setEnabled(true);
    ui->menuBar->setEnabled(true);
}

void LauncherWindow::showUpdateDialog(QString message)
{

    UpdateDialog* d = new UpdateDialog(message, this);
    d->exec();
    delete d;

    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
}

void LauncherWindow::playButtonClicked()
{
    logger->append(this->objectName(), "Try to start game...\n");
    logger->append(this->objectName(), "Client id: "
                   + settings->getClientStrId(settings->loadActiveClientId())
                   + "\n");

    QRect geometry = settings->loadClientWindowGeometry();

    gameRunner = new GameRunner(ui->nickEdit->text(),
                                ui->passEdit->text(),
                                QString( "NOT NOW" ),
                                !ui->playOffline->isChecked(),
                                 geometry );

    connect(gameRunner, SIGNAL(error(QString)),
            this,       SLOT(gameRunnerError(QString)));
    connect(gameRunner, SIGNAL(needUpdate(QString)),
            this,       SLOT(gameRunnerNeedUpdate(QString)));
    connect(gameRunner, SIGNAL(started()),
            this,       SLOT(gameRunnerStarted()));
    connect(gameRunner, SIGNAL(finished(int)),
            this,       SLOT(gameRunnerFinished(int)));
    //connect(this, &LauncherWindow::windowClosed,
            //gameRunner, &GameRunner::stopRunner);

    freezeInterface();
    gameRunner->startRunner();

}

void LauncherWindow::gameRunnerStarted()
{
    if (ui->hideLauncher->isChecked())
    {
        this->hide();
        logger->append(this->objectName(), "Main window hidden.\n");
    }
}

void LauncherWindow::gameRunnerError(const QString &message)
{
    delete gameRunner;
    unfreezeInterface();
    showError("Game run error!", message);
}

void LauncherWindow::gameRunnerNeedUpdate(const QString &message)
{
    delete gameRunner;
    unfreezeInterface();
    showUpdateDialog(message);
}

void LauncherWindow::gameRunnerFinished(int exitCode)
{
    delete gameRunner;

    if (this->isHidden())
    {
        this->show();
        logger->append(this->objectName(), "Main window showed\n");
    }

    unfreezeInterface();

    if (exitCode != 0)
        showError("Game run error!", "Process finished incorrectly!");
}

LauncherWindow::~LauncherWindow()
{
    delete ui;
}
