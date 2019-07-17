#include "storemanagedialog.h"
#include "../ui/ui_storemanagedialog.h"

#include "jsonparser.h"

StoreManageDialog::StoreManageDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StoreManageDialog)
{
    ui->setupUi(this);

    settings = Settings::instance();
    logger   = Logger::logger();
    ttyhstore = new QProcess();

    ui->log->setFont( QFontDatabase::systemFont(QFontDatabase::FixedFont) );

    connect(ui->cancelButton, &QPushButton::clicked,
            this, &StoreManageDialog::cancel);

    connect(ui->actionButton, &QPushButton::clicked,
            this, &StoreManageDialog::runCommand);

    // Connect process
    connect(ttyhstore, &QProcess::started,
            this, &StoreManageDialog::onStart);

    connect( ttyhstore, SIGNAL( finished(int) ),
             this, SLOT( onFinish(int) ) );

    connect(ttyhstore, &QProcess::readyReadStandardOutput,
            this, &StoreManageDialog::onOutput);

    connect(ttyhstore, &QProcess::readyReadStandardError,
            this, &StoreManageDialog::onOutput);

    connect( ttyhstore, SIGNAL( error(QProcess::ProcessError) ),
             this, SLOT( onError(QProcess::ProcessError) ) );

    // Connect fetcher
    connect(&fetcher, &DataFetcher::finished,
            this, &StoreManageDialog::onVersionsReply);

    // Connect command combo
    connect( ui->commandCombo, SIGNAL( currentIndexChanged(int) ),
             this, SLOT( onCommandSwitched(int) ) );

    QStringList commands;
    commands << "collect" << "clone" << "cleanup";
    ui->commandCombo->addItems(commands);

    requestVersions();
}

StoreManageDialog::~StoreManageDialog()
{
    if (ttyhstore->state() != QProcess::NotRunning)
    {
        ttyhstore->terminate();
        ttyhstore->waitForFinished();
    }

    delete ttyhstore;
    delete ui;
}

void StoreManageDialog::log(const QString &line, bool hidden)
{
    logger->appendLine(tr("StoreManageDialog"), line);
    if (!hidden)
    {
        ui->log->appendPlainText(line);
    }
}

void StoreManageDialog::setControlsEnabled(bool state)
{
    ui->commandCombo->setEnabled(state);
    ui->versionCombo->setEnabled(state);
    ui->actionButton->setEnabled(state);
}

void StoreManageDialog::onCommandSwitched(int id)
{
    Q_UNUSED(id);
    ui->versionCombo->setVisible(ui->commandCombo->currentText() == "clone");
}

void StoreManageDialog::requestVersions()
{
    log( tr("Requesting version list for 'clone'...") );
    fetcher.makeGet( QUrl( settings->getVanillaVersionsUrl() ) );
}

void StoreManageDialog::onVersionsReply(bool result)
{
    if (result)
    {
        log( tr("Versions list received. Parsing...") );

        JsonParser parser;
        if ( !parser.setJson( fetcher.getData() ) )
        {
            log( tr("Error: ") + parser.getParserError() );
        }
        else
        {
            if ( !parser.hasVersionList() )
            {
                log( tr("Error: reply not contain version list!") );
            }
            else
            {
                QStringList versions = parser.getReleaseVersonList();
                ui->versionCombo->addItems(versions);

                log( tr("Versions list for clone is ready!") );

                ui->versionCombo->setEnabled(true);
                ui->actionButton->setEnabled(true);
            }
        }
    }
    else
    {
        log( tr("Version list not received") );
    }
}

void StoreManageDialog::runCommand()
{
    QStringList args; args << "-v";
    args << "--root" << settings->loadStoreDirPath();

    QString command = ui->commandCombo->currentText();
    args << command;

    if (command == "clone")
    {
      QString version = ui->versionCombo->currentText();
      if ( version.isEmpty() )
      {
          log( tr("Error: version for cloning is empty!") );
          return;
      }
      else
      {
        args << version;
      }
    }

    setControlsEnabled(false);
    ui->log->clear();

    ttyhstore->setProgram( settings->loadStoreExePath() );
    ttyhstore->setArguments(args);
    ttyhstore->start();
}

void StoreManageDialog::cancel()
{
    if ( fetcher.isWaiting() )
    {
      fetcher.cancel();
    }

    if (ttyhstore->state() == QProcess::NotRunning)
    {
        this->close();
    }
    else
    {
        log( tr("ttyhstore cancelled by user") );
        ttyhstore->terminate();
    }
}

void StoreManageDialog::onStart()
{
    QString command = ui->commandCombo->currentText();
    if (command == "clone")
    {
        QString version = ui->versionCombo->currentText();
        log( QString("ttyhstore %1 %2").arg(command).arg(version) );
    }
    else
    {
        log( QString("ttyhstore %1").arg(command) );
    }
}

void StoreManageDialog::onFinish(int exitCode)
{
    setControlsEnabled(true);
    log( tr("ttyhstore finished with code %1.").arg(exitCode) );
}

void StoreManageDialog::onOutput()
{
    ui->log->appendPlainText( ttyhstore->readAll().trimmed() );
}

void StoreManageDialog::onError(QProcess::ProcessError error)
{
    Q_UNUSED(error);

    setControlsEnabled(true);

    QString message = ttyhstore->errorString();
    log( tr("Error: %1").arg(message) );
}
