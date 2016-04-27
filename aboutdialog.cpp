#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "settings.h"
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
}

void AboutDialog::showLicense() {
    LicenseDialog* d = new LicenseDialog(this);
    d->exec();
    delete d;
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
