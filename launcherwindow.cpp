#include "launcherwindow.h"
#include "ui_launcherwindow.h"

#include "settingsdialog.h"
#include "skinuploaddialog.h"
#include "updatedialog.h"
#include "feedbackdialog.h"
#include "aboutdialog.h"
#include "selfupdatedialog.h"
#include "storesettingsdialog.h"

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
    ui->setupUi(this);

    settings = Settings::instance();
    logger = Logger::logger();

    // Show welcome message
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
        log( tr("Can't open logo resource.") );
    }

    // Setup menu
    connect(ui->runSettings, &QAction::triggered, this,
            &LauncherWindow::showSettingsDialog);

    connect(ui->changeSkin, &QAction::triggered, this,
            &LauncherWindow::showSkinLoadDialog);

    connect(ui->runStoreSettings, &QAction::triggered, this,
            &LauncherWindow::showStoreSettingsDialog);

    connect(ui->updateManager, &QAction::triggered, this,
            &LauncherWindow::showUpdateManagerDialog);

    connect(ui->bugReport, &QAction::triggered, this,
            &LauncherWindow::showFeedBackDialog);

    connect(ui->aboutLauncher, &QAction::triggered, this,
            &LauncherWindow::showAboutDialog);

    bool isOffline = settings->loadOfflineModeState();
    ui->playOffline->setChecked(isOffline);
    offlineModeChanged();

    connect(ui->playOffline, &QAction::triggered, this,
            &LauncherWindow::offlineModeChanged);

    bool isHideWindow = settings->loadHideWindowModeState();
    ui->hideLauncher->setChecked(isHideWindow);

    connect(ui->hideLauncher, &QAction::triggered, this,
            &LauncherWindow::hideWindowModeChanged);

    bool isLoadNews = settings->loadNewsState();
    ui->loadNews->setChecked(isLoadNews);

    connect(ui->loadNews, &QAction::triggered, this,
            &LauncherWindow::fetchNewsModeChanged);

    connect(&newsFetcher, &DataFetcher::finished, this,
            &LauncherWindow::newsFetched);

    if ( settings->loadNewsState() )
    {
        newsFetcher.makeGet( QUrl(Settings::newsFeed) );
    }

    // Setup form
    QString login = settings->loadLogin();
    ui->nickEdit->setText(login);

    connect(ui->nickEdit, &QLineEdit::textChanged, settings,
            &Settings::saveLogin);

    bool isPassStored = settings->loadPassStoreState();
    ui->savePassword->setChecked(isPassStored);

    if (isPassStored)
    {
        QString password = settings->loadPassword();
        ui->passEdit->setText(password);
    }

    connect(ui->savePassword, &QCheckBox::clicked, settings,
            &Settings::savePassStoreState);

    QStringList clients = settings->getClientCaptions();
    ui->clientCombo->addItems(clients);

    int currentClient = settings->loadActiveClientID();
    ui->clientCombo->setCurrentIndex(currentClient);

    connect( ui->clientCombo, SIGNAL( activated(int) ), settings,
             SLOT( saveActiveClientID(int) ) );

    // Setup window parameters
    QRect geometry = settings->loadWindowGeometry();

    if (geometry.x() < 0)
    {
        QPoint scrCenter = QApplication::desktop()->screen()->rect().center();
        QPoint winCenter = this->rect().center();

        this->move(scrCenter - winCenter);
    }
    else
    {
        this->setGeometry(geometry);
    }

    if ( settings->loadMaximizedState() )
    {
        this->showMaximized();
    }

    // Other
    connect(ui->playButton, &QPushButton::clicked, this,
            &LauncherWindow::playButtonClicked);

    connect(logger, &Logger::lineAppended, this, &LauncherWindow::appendToLog);

#ifdef Q_OS_WIN
    QString latest = settings->getlatestVersion();

    if (Settings::launcherVersion != latest)
    {
        connect(this, &LauncherWindow::windowOpened, this, [=]()
        {
            QString msg = tr("New launcher version avialable: %1").arg(latest);

            SelfUpdateDialog *d = new SelfUpdateDialog(msg, this);
            d->exec();
            delete d;
        },
        Qt::QueuedConnection);
    }
#endif

    if (ui->clientCombo->count() == 0)
    {
        connect(this, &LauncherWindow::windowOpened, this, [=]()
        {
            ui->playButton->setEnabled(false);
            showError(tr("No available clients!"), true);
        },
        Qt::QueuedConnection);
    }
}

void LauncherWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    emit windowOpened();
    event->accept();
}

void LauncherWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
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
    QRegularExpression urlRegEx("(https?://[A-Za-z0-9\\.\\-\\?_=~#/]+)");
    QRegularExpressionMatch urlMatch = urlRegEx.match(line);

    if ( urlMatch.hasMatch() )
    {
        QString htmlLine = "";

        int current = 0;
        int last = line.length();
        int matches = urlMatch.lastCapturedIndex();

        for (int i = 1; i <= matches; i++)
        {
            int urlBegin = urlMatch.capturedStart(i);
            int urlEnd = urlMatch.capturedEnd(i);

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

            QString pat = "<a href=\"${url}\">${esc}</a>";
            QString esc = escapeString(url);

            htmlLine += pat.replace("${url}", url).replace("${esc}", esc);

            current = urlEnd;

            // Text after URL in last match
            if (i == matches && current < last)
            {
                int postLen = last - current;
                QString post = line.mid(urlEnd, postLen);
                htmlLine += escapeString(post);
            }
        }

        ui->logDisplay->appendHtml(htmlLine + "\n");
    }
    else
    {
        ui->logDisplay->appendHtml( escapeString(line) );

        // NOTE: plain text sometimes has URL apperance
        // ui->logDisplay->appendPlainText(line);
    }
}

QString LauncherWindow::escapeString(const QString &string)
{
    return string
           .toHtmlEscaped()
           .replace(" ", "&nbsp;")
           .replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
}

void LauncherWindow::showError(const QString &message, bool showInLog)
{
    if (showInLog)
    {
        log( tr("Error! %1").arg(message) );
    }

    QMessageBox::critical(this, tr("Oops! Error!"), message);
}

void LauncherWindow::log(const QString &line)
{
    logger->appendLine(tr("LauncherWindow"), line);
}

void LauncherWindow::storeParameters()
{
    settings->saveWindowGeometry( this->geometry() );
    settings->saveMaximizedState( this->isMaximized() );

    if ( ui->savePassword->isChecked() )
    {
        settings->savePassword( ui->passEdit->text() );
    }
    else
    {
        settings->savePassword("");
    }
}

void LauncherWindow::showSettingsDialog()
{
    SettingsDialog *d = new SettingsDialog(this);
    d->exec();
    delete d;

    ui->clientCombo->setCurrentIndex( settings->loadActiveClientID() );
}

void LauncherWindow::showSkinLoadDialog()
{
    SkinUploadDialog *d = new SkinUploadDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showStoreSettingsDialog()
{
    StoreSettingsDialog *d = new StoreSettingsDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showUpdateManagerDialog()
{
    showUpdateDialog( tr("Select a client, then press 'Check' button.") );
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
    bool isOffline = ui->playOffline->isChecked();
    settings->saveOfflineModeState(isOffline);

    if (isOffline)
    {
        ui->playButton->setText( tr("Play (offline)") );
    }
    else
    {
        ui->playButton->setText( tr("Play") );
    }
}

void LauncherWindow::hideWindowModeChanged()
{
    settings->saveHideWindowModeState( ui->hideLauncher->isChecked() );
}

void LauncherWindow::fetchNewsModeChanged()
{
    settings->saveNewsState( ui->loadNews->isChecked() );
}

void LauncherWindow::newsFetched(bool result)
{
    if (result)
    {
        appendToLog( newsFetcher.getData() );
    }
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

    ui->clientCombo->setCurrentIndex( settings->loadActiveClientID() );
}

void LauncherWindow::playButtonClicked()
{
    log( tr("Try to start game...") );

    QString client = settings->getClientName( settings->loadActiveClientID() );
    log( tr("Client: %1.").arg(client) );

    QString login = ui->nickEdit->text();
    QString pass = ui->passEdit->text();
    bool isOnline = !ui->playOffline->isChecked();
    QRect geometry = this->geometry();

    gameRunner = new GameRunner(login, pass, isOnline, geometry);

    connect(gameRunner, &GameRunner::error, this,
            &LauncherWindow::gameRunnerError);

    connect(gameRunner, &GameRunner::needUpdate, this,
            &LauncherWindow::gameRunnerNeedUpdate);

    connect(gameRunner, &GameRunner::started, this,
            &LauncherWindow::gameRunnerStarted);

    connect(gameRunner, &GameRunner::finished, this,
            &LauncherWindow::gameRunnerFinished);

    freezeInterface();
    gameRunner->Run();
}

void LauncherWindow::gameRunnerStarted()
{
    if ( ui->hideLauncher->isChecked() )
    {
        this->hide();
        log( tr("Main window hidden.") );
    }
}

void LauncherWindow::gameRunnerError(const QString &message)
{
    gameRunner->deleteLater();
    unfreezeInterface();
    showError(message, false);
}

void LauncherWindow::gameRunnerNeedUpdate(const QString &message)
{
    gameRunner->deleteLater();
    unfreezeInterface();
    showUpdateDialog(message);
}

void LauncherWindow::gameRunnerFinished(int exitCode)
{
    gameRunner->deleteLater();

    if ( this->isHidden() )
    {
        this->show();
        log( tr("Main window visible.") );
    }

    unfreezeInterface();

    if (exitCode != 0)
    {
        showError(tr("Process finished incorrectly!"), true);
    }
}

LauncherWindow::~LauncherWindow()
{
    delete ui;
}
