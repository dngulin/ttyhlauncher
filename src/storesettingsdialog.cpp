#include "storesettingsdialog.h"
#include "../ui/ui_storesettingsdialog.h"

#include <QFileDialog>

StoreSettingsDialog::StoreSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StoreSettingsDialog)
{
    ui->setupUi(this);

    settings = Settings::instance();
    logger = Logger::logger();

    loadSettings();

    connect(ui->saveButton, &QPushButton::clicked,
            this, &StoreSettingsDialog::saveSettings);

    connect(ui->exePathButton, &QPushButton::clicked,
            this, &StoreSettingsDialog::openSelectExeDialog);

    connect(ui->dirPathButton, &QPushButton::clicked,
            this, &StoreSettingsDialog::openSelectDirDialog);
}

StoreSettingsDialog::~StoreSettingsDialog()
{
    delete ui;
}

void StoreSettingsDialog::log(const QString &text)
{
    logger->appendLine(tr("StoreSettingsDialog"), text);
}

void StoreSettingsDialog::loadSettings()
{
    QString exe = settings->loadStoreExePath();
    QString dir = settings->loadStoreDirPath();

    log( tr("Load:") );
    log(tr("Store manager: ") + exe);
    log(tr("Store directory: ") + dir);

    ui->exePathEdit->setText(exe);
    ui->dirPathEdit->setText(dir);
}

void StoreSettingsDialog::saveSettings()
{
    QString exe = ui->exePathEdit->text();
    QString dir = ui->dirPathEdit->text();

    log( tr("Save:") );
    log(tr("Store manager: ") + exe);
    log(tr("Store directory: ") + dir);

    settings->saveStoreExePath(exe);
    settings->saveStoreDirPath(dir);
    this->close();
}

void StoreSettingsDialog::openSelectExeDialog()
{
    QString dir = QDir::homePath();
    QFileInfo exeFileInfo(ui->exePathEdit->text());
    if (exeFileInfo.exists())
    {
        dir = exeFileInfo.absoluteDir().absolutePath();
    }

    QString title = tr("Select the ttyhstore executable");
    QString path = QFileDialog::getOpenFileName(this, title, dir, "");

    if (!path.isEmpty())
    {
        ui->exePathEdit->setText(path);
    }
}

void StoreSettingsDialog::openSelectDirDialog()
{
    QString title = tr("Select a repository location");
    QString dir = ui->dirPathEdit->text();
    QString path = QFileDialog::getExistingDirectory(this, title, dir);

    if (!path.isEmpty())
    {
        ui->dirPathEdit->setText(path);
    };
}
