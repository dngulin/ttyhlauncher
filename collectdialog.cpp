#include "collectdialog.h"
#include "ui_collectdialog.h"

CollectDialog::CollectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CollectDialog)
{
    ui->setupUi(this);

    settings = Settings::instance();
    logger   = Logger::logger();

    ttyhstore.setProgram( settings->loadStoreExePath() );
    QStringList args;
    args << "--root" << settings->loadStoreDirPath();
    args << "collect";
    ttyhstore.setArguments(args);

    ui->log->setFont( QFontDatabase::systemFont(QFontDatabase::FixedFont) );
    ui->log->setPlainText( tr("Press 'Collect' button for build repository") );

    connect(ui->cancelButton, &QPushButton::clicked,
            this, &CollectDialog::cancel);

    connect(ui->collectButton, &QPushButton::clicked,
            this, &CollectDialog::collect);

    // Connect process
    connect(&ttyhstore, &QProcess::started,
            this, &CollectDialog::onStart);

    connect( &ttyhstore, SIGNAL( finished(int) ),
             this, SLOT( onFinish(int) ) );

    connect(&ttyhstore, &QProcess::readyReadStandardOutput,
            this, &CollectDialog::onOutput);

    connect(&ttyhstore, &QProcess::readyReadStandardError,
            this, &CollectDialog::onOutput);

    connect( &ttyhstore, SIGNAL( error(QProcess::ProcessError) ),
             this, SLOT( onError(QProcess::ProcessError) ) );

}

CollectDialog::~CollectDialog()
{
    delete ui;
}

void CollectDialog::log(const QString &line, bool hidden)
{
    logger->appendLine(tr("CollectDialog"), line);
    if (!hidden)
    {
        ui->log->appendPlainText(line);
    }
}

void CollectDialog::collect()
{
    ui->collectButton->setEnabled(false);
    ttyhstore.start();
}

void CollectDialog::cancel()
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

void CollectDialog::onStart()
{
    log( tr("ttyhstore collect") );
}

void CollectDialog::onFinish(int exitCode)
{
    ui->collectButton->setEnabled(true);
    log( tr("ttyhstore finished with code %1.").arg(exitCode) );
}

void CollectDialog::onOutput()
{
    ui->log->appendPlainText( ttyhstore.readAll().trimmed() );
}

void CollectDialog::onError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    ui->collectButton->setEnabled(true);

    QString message = ttyhstore.errorString();
    log( tr("Error: %1").arg(message) );
}
