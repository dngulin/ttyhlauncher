#include "checkoutdialog.h"
#include "ui_checkoutdialog.h"

CheckoutDialog::CheckoutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CheckoutDialog)
{
    ui->setupUi(this);
    settings = Settings::instance();
    logger = Logger::logger();

    ui->clientCombo->addItems(settings->getClientsNames());

    connect(ui->clientCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(makeVersionList()));
    makeVersionList();
}

CheckoutDialog::~CheckoutDialog() {
    delete ui;
}

void CheckoutDialog::makeCheckout() {

    if ( (ui->clientCombo->count() < 1) || (ui->versionCombo->count() < 1) ) {
        return;
    }

    ui->log->clear();
    errList.clear();
    errList << "-----" << "Список ошибок при расчёте контрольных сумм";

    ui->clientCombo->setEnabled(false);
    ui->versionCombo->setEnabled(false);
    ui->checkoutButton->setEnabled(false);

    // Prepare JSON objects
    QJsonObject dataObject;
    QJsonObject main, libs, files;

    // Setup main section
    QFile jarFile();

    // Setup libs section

    // Setup files section
    QJsonArray mutables;
    QJsonObject index;

    ui->clientCombo->setEnabled(true);
    ui->versionCombo->setEnabled(true);
    ui->checkoutButton->setEnabled(true);

}

void CheckoutDialog::makeVersionList() {
    // Remove old entries
    ui->versionCombo->clear();

    if (ui->clientCombo->count() > 0) {
        // make entries list
        QDir verDir = QDir(settings->getBaseDir() + "/client_"
                         + settings->getClientStrId(ui->clientCombo->currentIndex())
                         + "/versions");
        ui->versionCombo->addItems(verDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    } else {
        ui->log->appendPlainText("ВНИМАНИЕ: Пустой список клиентов");
        logger->append("CheckoutDialog", "WARN: Empty client list!\n");
    }
}
