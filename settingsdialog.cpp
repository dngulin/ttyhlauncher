#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>

#include "jsonparser.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    settings = Settings::instance();
    logger = Logger::logger();

    ui->clientCombo->addItems( settings->getClientCaptions() );
    ui->clientCombo->setCurrentIndex( settings->loadActiveClientID() );

    connect(&fetcher, &DataFetcher::finished,
            this, &SettingsDialog::makeVersionList);

    connect( ui->clientCombo, SIGNAL(currentIndexChanged(int)), settings,
             SLOT(saveActiveClientID(int)));

    connect( ui->clientCombo, SIGNAL(currentIndexChanged(int)), this,
             SLOT(loadSettings()));

    connect( ui->clientCombo, SIGNAL(currentIndexChanged(int)), this,
             SLOT(loadVersionList()));

    if (ui->clientCombo->count() == 0)
    {
        ui->saveButton->setEnabled(false);
        ui->opendirButton->setEnabled(false);

        msg( tr("Error: empty client list!") );
    }
    else
    {
        int id = ui->clientCombo->currentIndex();
        emit ui->clientCombo->currentIndexChanged(id);
    }

    connect(ui->javapathButton, &QPushButton::clicked,
            this, &SettingsDialog::chooseJavaPath);

    connect(ui->ksPathButton, &QPushButton::clicked,
            this, &SettingsDialog::chooseJavaPath);

    connect(ui->saveButton, &QPushButton::clicked,
            this, &SettingsDialog::saveSettings);

    connect(ui->opendirButton, &QPushButton::clicked,
            this, &SettingsDialog::openClientDirectory);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::msg(const QString &text)
{
    ui->stateEdit->setText(text);
    log(text);
}

void SettingsDialog::log(const QString &text)
{
    logger->appendLine(tr("SettingsDialog"), text);
}

void SettingsDialog::loadVersionList()
{
    msg( tr("Loading version list...") );

    ui->versionCombo->setEnabled(false);
    ui->versionCombo->clear();
    ui->versionCombo->addItem(tr("Latest version"), "latest");

    fetcher.makeGet( QUrl( settings->getVersionsUrl() ) );
}

void SettingsDialog::makeVersionList(bool result)
{
    if (!result)
    {
        log( tr("Error: ") + fetcher.errorString() );
        appendVersionList( tr("Local versions (server unreachable)") );
    }
    else
    {
        JsonParser parser;
        if ( !parser.setJson( fetcher.getData() ) )
        {
            log( tr("Error: ") + parser.getParserError() );
            appendVersionList( tr("Local versions (bad server reply)") );
        }
        else
        {
            if ( !parser.hasVersionList() )
            {
                log( tr("Error: no version list!") );
                appendVersionList( tr("Local versions (empty server reply)") );
            }
            else
            {
                foreach ( QString id, parser.getReleaseVersonList() )
                {
                    QString title = id;
                    if ( isVersionInstalled(id) )
                    {
                        title += tr(" [prefix installed]");
                    }
                    ui->versionCombo->addItem(title, id);
                }
                appendVersionList( tr("Version list from update server") );
            }
        }
    }
}

void SettingsDialog::appendVersionList(const QString &reason)
{
    log( tr("Append local version list...") );
    ui->stateEdit->setText(reason);

    QString prefix = settings->getClientDir() + "/versions";

    QDir verdir = QDir(prefix);
    QStringList subdirs = verdir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (QString id, subdirs)
    {
        QFile file(prefix + "/" + id + "/" + id + ".json");
        if ( file.exists() )
        {
            if (ui->versionCombo->findData(id) == -1)
            {
                QString title = id + tr(" [local]");
                if ( isVersionInstalled(id) )
                {
                    title += tr(" [prefix installed]");
                }
                ui->versionCombo->addItem(title, id);
            }
        }
    }

    int id = ui->versionCombo->findData( settings->loadClientVersion() );
    if (id != -1)
    {
        ui->versionCombo->setCurrentIndex(id);
    }
    ui->versionCombo->setEnabled(true);
}

void SettingsDialog::logCurrentSettings()
{
    QString yes = tr("true");
    QString no = tr("false");

    QString client = settings->getClientName( settings->loadActiveClientID() );

    log(tr("\tClient: ") + client);
    log( tr("\tVersion: ") + settings->loadClientVersion() );

    log( tr("\tUseCustomJava: ") + (ui->javapathBox->isChecked() ? yes : no) );
    log( tr("\tCustomJava: ") + ui->javapathEdit->text() );
    log( tr("\tUseCustomArgs: ") + (ui->argsBox->isChecked() ? yes : no) );
    log( tr("\tCustomArgs: ") + ui->argsEdit->text() );

    log( tr("\tSetWindowGeometry: ") + (ui->sizeBox->isChecked() ? yes : no) );
    log( tr("\tCustomGeometry: ")
         + QString::number( ui->widthSpinBox->value() ) + ","
         + QString::number( ui->heightSpinBox->value() ) );

    log( tr("\tMakeFullscreen: ")
         + (ui->fullscreenRadio->isChecked() ? yes : no) );
    log( tr("\tUseLauncherSize: ")
         + (ui->useLauncherRadio->isChecked() ? yes : no) );

    log( tr("\tCheckAssets: ")
         + (ui->checkAssetsCombo->isChecked() ? yes : no) );

    log( tr("\tUseJavaKeystore: ")
         + (ui->keystoreBox->isChecked() ? yes : no) );
}

bool SettingsDialog::isVersionInstalled(const QString &name)
{
    QString prefix = settings->getClientDir() + "/prefixes/";
    return QFile(prefix + name + "/installed_data.json").exists();
}

void SettingsDialog::saveSettings()
{
    int id = ui->versionCombo->currentIndex();
    QString version = ui->versionCombo->itemData(id).toString();
    settings->saveClientVersion(version);

    settings->saveClientJavaState( ui->javapathBox->isChecked() );
    settings->saveClientJava( ui->javapathEdit->text() );
    settings->saveClientJavaArgsState( ui->argsBox->isChecked() );
    settings->saveClientJavaArgs( ui->argsEdit->text() );

    QRect g( -1, -1, ui->widthSpinBox->value(), ui->heightSpinBox->value() );
    settings->saveClientWindowGeometry(g);

    settings->saveClientWindowSizeState( ui->sizeBox->isChecked() );
    settings->saveClientFullscreenState( ui->fullscreenRadio->isChecked() );

    bool launcherSizeState = ui->useLauncherRadio->isChecked();
    settings->saveClientUseLauncherSizeState(launcherSizeState);

    bool ksState = ui->keystoreBox->isChecked();
    settings->saveClientJavaKeystoreState(ksState);
    settings->saveClientJavaKeystorePath( ui->ksPathEdit->text() );
    settings->saveClientJavaKeystorePass( ui->ksPassEdit->text() );

    settings->saveClientCheckAssetsState( ui->checkAssetsCombo->isChecked() );

    log( tr("Settings saved:") );
    logCurrentSettings();
    this->close();
}

void SettingsDialog::loadSettings()
{
    // Setup settings
    ui->javapathBox->setChecked( settings->loadClientJavaState() );
    ui->javapathEdit->setText( settings->loadClientJava() );
    ui->argsBox->setChecked( settings->loadClientJavaArgsState() );
    ui->argsEdit->setText( settings->loadClientJavaArgs() );

    QRect g = settings->loadClientWindowGeometry();
    ui->widthSpinBox->setValue( g.width() );
    ui->heightSpinBox->setValue( g.height() );

    ui->sizeBox->setChecked( settings->loadClientWindowSizeState() );

    bool fullscreen = settings->loadClientFullscreenState();
    bool useLauncherSize = settings->loadClientUseLauncherSizeState();
    ui->fullscreenRadio->setChecked(fullscreen);
    ui->useLauncherRadio->setChecked(useLauncherSize);
    if (!fullscreen && !useLauncherSize)
    {
        ui->customSizeRadio->setChecked(true);
    }

    bool useKeystore = settings->loadClientJavaKeystoreState();
    ui->keystoreBox->setChecked(useKeystore);

    ui->ksPathEdit->setText( settings->loadClientJavaKeystorePath() );
    ui->ksPassEdit->setText( settings->loadClientJavaKeystorePass() );

    bool checkAssets = settings->loadClientCheckAssetsState();
    ui->checkAssetsCombo->setChecked(checkAssets);

    log( tr("Settings loaded:") );
    logCurrentSettings();
}

void SettingsDialog::chooseJavaPath()
{
    QString title = tr("Select a java executable");
    QString javapath = QFileDialog::getOpenFileName(this, title, "", "");
    ui->javapathEdit->setText(javapath);
}

void SettingsDialog::chooseJavaKeystore()
{
    QString title = tr("Select a java keystore file");
    QString javapath = QFileDialog::getOpenFileName(this, title, "", "");
    ui->ksPathEdit->setText(javapath);
}

void SettingsDialog::openClientDirectory()
{
    QFile clientDir( settings->getClientDir() );
    if ( clientDir.exists() )
    {
        QUrl clientDirUrl = QUrl::fromLocalFile( clientDir.fileName() );
        QDesktopServices::openUrl(clientDirUrl);
    }
    else
    {
        QString head = tr("Oops! We have a problem!");
        QString msg = tr("Can't open client directory! %1");

        log( msg.arg( clientDir.fileName() ) );
        QMessageBox::critical( this, head, msg.arg( clientDir.fileName() ) );
    }
}
