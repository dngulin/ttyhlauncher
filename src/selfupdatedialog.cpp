#include "selfupdatedialog.h"
#include "../data/ui_selfupdatedialog.h"

#include <QStandardPaths>
typedef QStandardPaths Path;

#include <QMessageBox>
#include <QApplication>

#include "util.h"
#include "settings.h"
#include "logger.h"

SelfUpdateDialog::SelfUpdateDialog(const QString &text, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SelfUpdateDialog)
{
    ui->setupUi(this);

    QString arch = Settings::instance()->getWordSize();
    QString server = Settings::buildServer;
    QString url = server + "/build-" + arch + "-latest/ttyhlauncher.zip";

    QString temp = Path::writableLocation(Path::TempLocation);
    QString path = temp + "/ttyhlauncher.zip";

    msg(text);

    fetcher.add(QUrl(url), path);

    ui->updateButton->setEnabled(true);
    ui->cancelButton->setEnabled(false);

    connect(ui->updateButton, &QPushButton::clicked,
            this, &SelfUpdateDialog::updateClicked);

    connect(ui->cancelButton, &QPushButton::clicked,
            this, &SelfUpdateDialog::cancelClicked);

    connect(&fetcher, &FileFetcher::sizesFetchResult,
            this, &SelfUpdateDialog::fetchSizeFinished);

    connect(&fetcher, &FileFetcher::filesFetchProgress,
            ui->progressBar, &QProgressBar::setValue);

    connect(&fetcher, &FileFetcher::filesFetchResult,
            this, &SelfUpdateDialog::downloadFinished);
}

SelfUpdateDialog::~SelfUpdateDialog()
{
    delete ui;
}

void SelfUpdateDialog::log(const QString &text)
{
    Logger::logger()->appendLine(tr("SelfUpdateDialog"), text);
}

void SelfUpdateDialog::msg(const QString &text)
{
    log(text);
    ui->label->setText(text);
}

void SelfUpdateDialog::updateClicked()
{
    msg( tr("Requesting download size...") );

    fetcher.fetchSizes();

    ui->cancelButton->setEnabled(true);
    ui->updateButton->setEnabled(false);

    ui->progressBar->setValue(0);
}

void SelfUpdateDialog::cancelClicked()
{
    msg( tr("Download cancelled.") );

    fetcher.cancel();

    ui->cancelButton->setEnabled(false);
    ui->updateButton->setEnabled(true);
}

void SelfUpdateDialog::fetchSizeFinished(bool result)
{
    if (!result)
    {
        msg( tr("Error! Can't determinate daownload size!") );

        ui->cancelButton->setEnabled(false);
        ui->updateButton->setEnabled(true);
    }
    else
    {
        double size = fetcher.getFetchSize() / 1024;

        QString unit = tr("KiB");
        if (size > 1024 * 1024)
        {
            size = size / (1024 * 1024);
            unit = tr("GiB");
        }
        else if (size > 1024)
        {
            size = size / 1024;
            unit = tr("MiB");
        }
        QString dsize = QString::number(size, 'f', 2);

        msg( tr("Downloading launcher (%1 %2)...").arg(dsize, unit) );

        fetcher.fetchFiles();
    }
}

void SelfUpdateDialog::downloadFinished(bool result)
{
    ui->cancelButton->setEnabled(false);

    if (!result)
    {
        ui->updateButton->setEnabled(true);
        msg( tr("Download failed!") );
    }
    else
    {
        QString tempDir = Path::writableLocation(Path::TempLocation);
        QString zipPath = tempDir + "/ttyhlauncher.zip";

        Util::unzipArchive(zipPath, tempDir);
        QFile::remove(zipPath);

        log( tr("Restarting launcher...") );


        QString title = tr("Complete");
        QString text = tr("Download completed. Launcher will be restarted.");
        QMessageBox::information(this, title, text);

        QString temp = tempDir + "/ttyhlauncher.exe";
        QString orig = QApplication::applicationFilePath();

        if ( QProcess::startDetached(temp, QStringList() << "-u" << orig) )
        {
            QApplication::exit(0);
        }
        else
        {
            QString title = tr("Update error");
            QString text = tr("Can't run temporary instance!");

            log( tr("Error! %1").arg(text) );

            QMessageBox::critical(this, title, text);
        }

        this->close();
    }
}
