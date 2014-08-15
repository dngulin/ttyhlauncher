#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "settings.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->info->setPlainText("ttyhlauncher " + Settings::launcherVerion + "\n\n" +
                           "Это ПО с открытым исходным кодом, распространяемое под лицензией " +
                           "GNU General Public License, версия 3.\n\n" +
                           "https://github.com/figec/ttyhlauncher");
    connect(ui->okButton, SIGNAL(clicked()), this, SLOT(close()));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
