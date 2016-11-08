#include "clonedialog.h"
#include "ui_clonedialog.h"

#include "jsonparser.h"

CloneDialog::CloneDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CloneDialog)
{
    ui->setupUi(this);

    settings = Settings::instance();
    logger   = Logger::logger();

    ui->log->setFont( QFontDatabase::systemFont(QFontDatabase::FixedFont) );

    connect(ui->cancelButton, &QPushButton::clicked,
            this, &CloneDialog::cancel);

    connect(ui->cloneButton, &QPushButton::clicked,
            this, &CloneDialog::clone);

    // Connect process
    connect(&ttyhstore, &QProcess::started,
            this, &CloneDialog::onStart);

    connect( &ttyhstore, SIGNAL( finished(int) ),
             this, SLOT( onFinish(int) ) );

    connect(&ttyhstore, &QProcess::readyReadStandardOutput,
            this, &CloneDialog::onOutput);

    connect(&ttyhstore, &QProcess::readyReadStandardError,
            this, &CloneDialog::onOutput);

    connect( &ttyhstore, SIGNAL( error(QProcess::ProcessError) ),
             this, SLOT( onError(QProcess::ProcessError) ) );

    // Connect fetcher
    connect(&fetcher, &DataFetcher::finished,
            this, &CloneDialog::onVersionsReply);

    requestVersions();

    ui->versionCombo->setEnabled(false);
    ui->cloneButton->setEnabled(false);
}

CloneDialog::~CloneDialog()
{
    delete ui;
}

void CloneDialog::log(const QString &line, bool hidden)
{
    logger->appendLine(tr("CloneDialog"), line);
    if (!hidden)
    {
        ui->log->appendPlainText(line);
    }
}

void CloneDialog::requestVersions()
{
    log( tr("Requesting version list...") );
    fetcher.makeGet( QUrl( settings->getVanillaVersionsUrl() ) );
}

void CloneDialog::onVersionsReply(bool result)
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

                log( tr("Versions list ready!") );

                ui->versionCombo->setEnabled(true);
                ui->cloneButton->setEnabled(true);
            }
        }
    }
    else
    {
        log( tr("Version list not received") );
    }
}

void CloneDialog::clone()
{
    ui->versionCombo->setEnabled(false);
    ui->cloneButton->setEnabled(false);

    QStringList args;
    args << "-v";
    args << "--root" << settings->loadStoreDirPath();
    args << "clone";
    args << ui->versionCombo->currentText();

    ttyhstore.setProgram( settings->loadStoreExePath() );
    ttyhstore.setArguments(args);
    ttyhstore.start();
}

void CloneDialog::cancel()
{
    if (ttyhstore.state() == QProcess::NotRunning)
    {
        this->close();
    }
    else
    {
        log( tr("ttyhstore cancelled by user") );
        ttyhstore.kill();
    }
}

void CloneDialog::onStart()
{
    QString version = ui->versionCombo->currentText();
    log( tr("ttyhstore clone %1").arg(version) );
}

void CloneDialog::onFinish(int exitCode)
{
    ui->versionCombo->setEnabled(true);
    ui->cloneButton->setEnabled(true);

    log( tr("ttyhstore finished with code %1.").arg(exitCode) );
}

void CloneDialog::onOutput()
{
    ui->log->appendPlainText( ttyhstore.readAll().trimmed() );
}

void CloneDialog::onError(QProcess::ProcessError error)
{
    Q_UNUSED(error);

    ui->versionCombo->setEnabled(true);
    ui->cloneButton->setEnabled(true);

    QString message = ttyhstore.errorString();
    log( tr("Error: %1").arg(message) );
}
