#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "settings.h"
#include "logger.h"
#include "licensedialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->ttyhlauncherLabel->setText("ttyhlauncher " + Settings::launcherVersion);

    ui->linkLabel->setOpenExternalLinks(true); // open link in external browser

    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->licenseButton, SIGNAL(clicked()), this, SLOT(showLicense()));

    Logger::logger()->append("AboutDialog", "About dialog opened\n");
}

void AboutDialog::showLicense() {
    LicenseDialog* d = new LicenseDialog(this);
    d->exec();
    delete d;
}

AboutDialog::~AboutDialog()
{
    Logger::logger()->append("AboutDialog", "About dialog closed\n");
    delete ui;
}
