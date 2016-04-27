#include "launcherwindow.h"
#include "ui_launcherwindow.h"

#include "settingsdialog.h"
#include "skinuploaddialog.h"
#include "updatedialog.h"
#include "feedbackdialog.h"
#include "aboutdialog.h"

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

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->logDisplay->setFont(font);

    appendToLog( tr("Welcome to the ttyhlauncher.") );

    QFile logoFile(":/resources/logo.txt");
    if ( logoFile.open(QFile::ReadOnly | QFile::Text) )
    {
        appendToLog( QTextStream(&logoFile).readAll() );
        logoFile.close();
    }
    else
    {
        logger->appendLine( this->objectName(), tr("Can't open logo resource.") );
    }

    connect( logger, SIGNAL( lineAppended(QString) ), this,
             SLOT( appendToLog(QString) ) );

    // Options Menu connections
    connect( ui->runSettings, SIGNAL( triggered() ), SLOT(
                  showSettingsDialog() ) );

    // Additional Menu connections
    connect( ui->changeSkin, SIGNAL( triggered() ),
             SLOT( showSkinLoadDialog() ) );
    connect( ui->updateManager, SIGNAL( triggered() ),
             SLOT( showUpdateManagerDialog() ) );

    // Help Menu connections
    connect( ui->bugReport, SIGNAL( triggered() ),
             SLOT( showFeedBackDialog() ) );
    connect( ui->aboutLauncher, SIGNAL( triggered() ),
             SLOT( showAboutDialog() ) );

    // Setup offlineMode entry
    ui->playOffline->setChecked( settings->loadOfflineModeState() );
    this->offlineModeChanged();
    connect( ui->playOffline, SIGNAL( triggered() ), SLOT(
                  offlineModeChanged() ) );

    // Setup hidewindow entry
    ui->hideLauncher->setChecked( settings->loadHideWindowModeState() );
    this->hideWindowModeChanged();
    connect( ui->hideLauncher, SIGNAL( triggered() ),
             SLOT( hideWindowModeChanged() ) );

    // Setup login field
    ui->nickEdit->setText( settings->loadLogin() );
    // Save login when changed
    connect( ui->nickEdit, SIGNAL( textChanged(QString) ), settings,
             SLOT( saveLogin(QString) ) );

    // Setup password field
    ui->savePassword->setChecked( settings->loadPassStoreState() );
    if ( ui->savePassword->isChecked() )
    {
        ui->passEdit->setText( settings->loadPassword() );
    }
    // Password are saved on login or exit if savePassword is checked
    connect( ui->savePassword, SIGNAL( clicked(bool) ), settings,
             SLOT( savePassStoreState(bool) ) );

    // Setup client combobox
    ui->clientCombo->addItems( settings->getClientsNames() );
    ui->clientCombo->setCurrentIndex( settings->loadActiveClientId() );
    connect( ui->clientCombo, SIGNAL( activated(int) ), settings,
             SLOT( saveActiveClientId(int) ) );

    // Setup window parameters
    QRect geometry = settings->loadWindowGeometry();
    // Centering window, if loaded default values
    if (geometry.x() < 0)
    {
        this->move(
             QApplication::desktop()->screen()->rect().center()
            - this->rect().center() );
    }
    else
    {
        this->setGeometry(geometry);
    }

    // Restore maximized state
    if ( settings->loadMaximizedState() )
    {
        this->showMaximized();
    }

    connect( ui->playButton, SIGNAL( clicked() ), this,
             SLOT( playButtonClicked() ) );

    if (ui->clientCombo->count() == 0)
    {
        this->show(); // hack to show window before error message
        ui->playButton->setEnabled(false);
        QMessageBox::critical(this, "Беда-беда!",
                              "Не удалось получить список клиентов\nМы все умрём.\n");
    }
}

void LauncherWindow::closeEvent(QCloseEvent *event)
{
    emit windowClosed();
    storeParameters();
    event->accept();
}

void LauncherWindow::keyPressEvent(QKeyEvent *pe)
{
    if (pe->key() == Qt::Key_Return)
    {
        playButtonClicked();
    }
    pe->accept();
}

void LauncherWindow::appendToLog(const QString &text)
{
    QStringList lines = text.split("\n");

    foreach (QString line, lines)
    {
        appendLineToLog(line);
    }
}

void LauncherWindow::appendLineToLog(const QString &line)
{
    QRegularExpression urlRegEx("((?:https?)://\\S+)");
    QRegularExpressionMatch urlMatch = urlRegEx.match(line);

    if ( urlMatch.hasMatch() )
    {
        QString htmlLine;

        int current = 0;
        int last  = line.length() - 1;
        int matches = urlMatch.lastCapturedIndex();

        for (int i = 1; i <= matches; i++)
        {
            int urlBegin = urlMatch.capturedStart(i);
            int urlEnd   = urlMatch.capturedEnd(i);

            // Text before URL
            if (current < urlBegin)
            {
                int preLen = urlBegin - current;
                QString pre = line.mid(current, preLen);
                htmlLine += escapeString(pre);
            }

            // URL
            int urlLen = urlEnd - urlBegin;
            QString url = line.mid(urlBegin, urlLen);
            htmlLine += QString("<a href='*'>*</a>\n").replace("*", url);

            current = urlEnd;

            // Text after URL in last match
            if ( i == matches && current < last )
            {
                int postLen = last - current;
                QString post = line.mid(urlEnd, postLen);
                htmlLine += escapeString(post);
            }
        }

        ui->logDisplay->appendHtml(htmlLine);
    }
    else
    {
        ui->logDisplay->appendPlainText(line);
    }

}

QString LauncherWindow::escapeString(const QString &string)
{
    return string.toHtmlEscaped().replace(" ", "&nbsp;");
}

void LauncherWindow::showError(const QString &title, const QString &message)
{
    QMessageBox::critical(this, title, message);
}

// Run this method on close window and run game
void LauncherWindow::storeParameters()
{
    settings->saveWindowGeometry( this->geometry() );
    settings->saveMaximizedState( this->isMaximized() );

    if ( ui->savePassword->isChecked() )
    {
        settings->savePassword( ui->passEdit->text() );
    }
    // Security issue
    else
    {
        settings->savePassword("");
    }
}

// Show dialog slots
void LauncherWindow::showSettingsDialog()
{
    SettingsDialog *d = new SettingsDialog(this);
    d->exec();
    delete d;

    ui->clientCombo->setCurrentIndex( settings->loadActiveClientId() );
}

void LauncherWindow::showSkinLoadDialog()
{
    SkinUploadDialog *d = new SkinUploadDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showUpdateManagerDialog()
{
    showUpdateDialog(
        "Для проверки наличия обновлений выберите нужный клиент и нажмите кнопку \"Проверить\"");
}

void LauncherWindow::showFeedBackDialog()
{
    FeedbackDialog *d = new FeedbackDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showAboutDialog()
{
    AboutDialog *d = new AboutDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::offlineModeChanged()
{
    settings->saveOfflineModeState( ui->playOffline->isChecked() );
    settings->loadOfflineModeState() ? ui->playButton->setText(
        "Играть (оффлайн)")
    : ui->playButton->setText("Играть");
}

void LauncherWindow::hideWindowModeChanged()
{
    settings->saveHideWindowModeState( ui->hideLauncher->isChecked() );
}

void LauncherWindow::freezeInterface()
{
    ui->runPanel->setEnabled(false);
    ui->menuBar->setEnabled(false);
}

void LauncherWindow::unfreezeInterface()
{
    ui->runPanel->setEnabled(true);
    ui->menuBar->setEnabled(true);
}

void LauncherWindow::showUpdateDialog(QString message)
{
    UpdateDialog *d = new UpdateDialog(message, this);
    d->exec();
    delete d;

    ui->clientCombo->setCurrentIndex( settings->loadActiveClientId() );
}

void LauncherWindow::playButtonClicked()
{
    logger->appendLine(this->objectName(), "Try to start game...");
    logger->appendLine(this->objectName(), "Client id: "
                   + settings->getClientStrId( settings->loadActiveClientId() ) );

    QRect geometry = settings->loadClientWindowGeometry();

    gameRunner = new GameRunner(ui->nickEdit->text(),
                                ui->passEdit->text(),
                                QString("NOT NOW"),
                                !ui->playOffline->isChecked(),
                                geometry);

    connect( gameRunner, SIGNAL( error(QString) ),
             this, SLOT( gameRunnerError(QString) ) );
    connect( gameRunner, SIGNAL( needUpdate(QString) ),
             this, SLOT( gameRunnerNeedUpdate(QString) ) );
    connect( gameRunner, SIGNAL( started() ),
             this, SLOT( gameRunnerStarted() ) );
    connect( gameRunner, SIGNAL( finished(int) ),
             this, SLOT( gameRunnerFinished(int) ) );
    // connect(this, &LauncherWindow::windowClosed,
    // gameRunner, &GameRunner::stopRunner);

    freezeInterface();
    gameRunner->startRunner();
}

void LauncherWindow::gameRunnerStarted()
{
    if ( ui->hideLauncher->isChecked() )
    {
        this->hide();
        logger->appendLine(this->objectName(), "Main window hidden.");
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

    if ( this->isHidden() )
    {
        this->show();
        logger->appendLine(this->objectName(), "Main window showed");
    }

    unfreezeInterface();

    if (exitCode != 0)
    {
        showError("Game run error!", "Process finished incorrectly!");
    }
}

LauncherWindow::~LauncherWindow()
{
    delete ui;
}
