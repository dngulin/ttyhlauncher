#include "aboutdialog.h"
#include "../data/ui_aboutdialog.h"

#include "settings.h"
#include "licensedialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->ttyhlauncherLabel->setText("ttyhlauncher " + Settings::launcherVersion);

    ui->linkLabel->setOpenExternalLinks(true);

    connect(ui->closeButton, &QPushButton::clicked, this, &AboutDialog::close);
    connect(ui->licenseButton, &QPushButton::clicked, this,
            &AboutDialog::showLicense);
}

void AboutDialog::showLicense()
{
    LicenseDialog *d = new LicenseDialog(this);
    d->exec();
    delete d;
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
