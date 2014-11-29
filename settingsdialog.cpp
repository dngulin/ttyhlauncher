#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    settings = Settings::instance();
    logger = Logger::logger();

    logger->append("SettingsDialog", "Settings dialog opened\n");

    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(makeVersionList(QNetworkReply*)));

    // Setup client combobox
    ui->clientCombo->addItems(settings->getClientsNames());
    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
    connect(ui->clientCombo, SIGNAL(activated(int)), settings, SLOT(saveActiveClientId(int)));
    connect(ui->clientCombo, SIGNAL(activated(int)), this, SLOT(loadSettings()));
    connect(ui->clientCombo, SIGNAL(activated(int)), this, SLOT(loadVersionList()));

    if (ui->clientCombo->count() == 0) {
        ui->saveButton->setEnabled(false);
        ui->opendirButton->setEnabled(false);

        logger->append("SettingsDialog", "Error: empty client list!\n");
        ui->stateEdit->setText("Не удалось получить список клиентов");
    } else {
        emit ui->clientCombo->activated(ui->clientCombo->currentIndex());
    }

    connect(ui->javapathButton, SIGNAL(clicked()), this, SLOT(openFileDialog()));
    connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
    connect(ui->opendirButton, SIGNAL(clicked()), this, SLOT(openClientDirectory()));
}

SettingsDialog::~SettingsDialog()
{
    logger->append("SettingsDialog", "Settings dialog closed\n");
    delete ui;
}


void SettingsDialog::loadVersionList() {
    ui->stateEdit->setText("Составляется список версий...");
    logger->append("SettingsDialog", "Making version list...\n");

    ui->versionCombo->setEnabled(false);
    ui->versionCombo->clear();
    ui->versionCombo->addItem("Последняя доступная версия", "latest");

    QNetworkRequest request;
    // FIXME: in release url depended at activeClient value
    request.setUrl(QUrl(settings->getVersionsUrl()));
    logger->append("SettingsDialog", "Making version list request...\n");
    logger->append("SettingsDialog", "URL: " + settings->getVersionsUrl() +"\n");
    nam->get(request);
}

void SettingsDialog::makeVersionList(QNetworkReply* reply) {

    // Check for connection error
    if (reply->error() == QNetworkReply::NoError) {
        logger->append("SettingsDialog", "OK\n");

        QByteArray rawResponce = reply->readAll();
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(rawResponce, &error);

        // Check for incorrect JSON
        if (error.error == QJsonParseError::NoError) {

            QJsonObject responce = json.object();

            // Check for error in server answer
            if (responce["error"].toString() != "") {
                // Error in answer handler
                ui->stateEdit->setText("Ошибка! " + responce["errorMessage"].toString());
                logger->append("SettingsDialog", "Error: "
                               + responce["errorMessage"].toString() + "\n");

            } else {
                // Correct login
                logger->append("SettingsDialog", "List downloaded\n");

                // Make remote version list
                QJsonArray versions = responce["versions"].toArray();

                foreach (QJsonValue value, versions) {
                    QJsonObject version = value.toObject();
                    if (version["type"].toString() == "release")
                        ui->versionCombo->addItem(version["id"].toString(), version["id"].toString());
                }

                // Make additional local version list
                appendVersionList("Список версий с сервера обновлений");
            }

        } else {
            // JSON parse error
            logger->append("SettingsDialog", "JSON parse error!\n");
            appendVersionList("Локальные версии (ошибка обмена с севрером)");
        }


    } else {
        // Connection error
        logger->append("SettingsDialog", "Error: " + reply->errorString() +"\n");
        appendVersionList("Локальные версии (сервер недоступен)");
    }

}

void SettingsDialog::appendVersionList(QString reason) {
    logger->append("SettingsDialog", "Append local version list...\n");
    ui->stateEdit->setText(reason);

    QString prefix = settings->getClientDir() + "/versions";
    QDir verdir = QDir(prefix);
    QStringList subdirs = verdir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (QString ver, subdirs) {
        QFile* file = new QFile(prefix + "/" + ver + "/" + ver + ".json");
        if (file->exists()) {
            // Add to version list unique local versions
            if (ui->versionCombo->findData(ver) == -1) {
                ui->versionCombo->addItem(ver + " (локальная версия)", ver);
            }
        }
        delete file;
    }

    int id = ui->versionCombo->findData(settings->loadClientVersion());
    if (id != -1) ui->versionCombo->setCurrentIndex(id);
    ui->versionCombo->setEnabled(true);
}

void SettingsDialog::saveSettings() {

    int id = ui->versionCombo->currentIndex();
    QString strid = ui->versionCombo->itemData(id).toString();
    settings->saveClientVersion(strid);

    settings->saveClientJavaState(ui->javapathBox->isChecked());
    settings->saveClientJava(ui->javapathEdit->text());
    settings->saveClientJavaArgsState(ui->argsBox->isChecked());
    settings->saveClientJavaArgs(ui->argsEdit->text());
    settings->saveClientWindowGeometry(QRect(-1, -1, ui->widthSpinBox->value(), ui->heightSpinBox->value()));
    settings->saveClientSizeState(ui->sizeBox->isChecked());
    settings->saveClientFullscreenState(ui->fullscreenRadio->isChecked());
    settings->saveUseLauncherSizeState(ui->useLauncherRadio->isChecked());

    logger->append("SettingsDialog", "Settings saved\n");
    logger->append("SettingsDialog", "\tClient: " + settings->getClientStrId(settings->loadActiveClientId()) + "\n");
    logger->append("SettingsDialog", "\tVersion: " + settings->loadClientVersion() + "\n");
    logger->append("SettingsDialog", "\tUseClientJava: " + QString(ui->javapathBox->isChecked() ? "true" : "false") + "\n");
    logger->append("SettingsDialog", "\tClientJava: " + ui->javapathEdit->text() + "\n");
    logger->append("SettingsDialog", "\tUseClientArgs: " + QString(ui->argsBox->isChecked() ? "true" : "false") + "\n");
    logger->append("SettingsDialog", "\tClientArgs: " + ui->argsEdit->text() + "\n");
    logger->append("SettingsDialog", "\tMinecraftGeometry: " +
                   QString::number(settings->loadClientWindowGeometry().width()) + "," +
                   QString::number(settings->loadClientWindowGeometry().height()) + "\n");
    logger->append("SettingsDialog", "\tFullscreen: " + QString(ui->fullscreenRadio->isChecked() ? "true" : "false")+"\n");
    logger->append("SettingsDialog", "\tUseLauncherSize: " + QString(ui->useLauncherRadio->isChecked() ? "true" : "false")+"\n");
    this->close();

}

void SettingsDialog::loadSettings() {

    // Setup settings
    ui->javapathBox->setChecked(settings->loadClientJavaState());
    ui->javapathEdit->setText(settings->loadClientJava());
    ui->argsBox->setChecked(settings->loadClientJavaArgsState());
    ui->argsEdit->setText(settings->loadClientJavaArgs());
    ui->widthSpinBox->setValue(settings->loadClientWindowGeometry().width());
    ui->heightSpinBox->setValue(settings->loadClientWindowGeometry().height());
    ui->sizeBox->setChecked(settings->loadClientSizeState());

    bool fullscreen = settings->loadClientFullscreenState();
    bool useLauncherSize = settings->loadUseLauncherSizeState();
    ui->fullscreenRadio->setChecked(fullscreen);
    ui->useLauncherRadio->setChecked(useLauncherSize);
    if(!fullscreen && !useLauncherSize) {
        ui->customSizeRadio->setChecked(true);
    }

    logger->append("SettingsDialog", "Settings loaded\n");
    logger->append("SettingsDialog", "\tClient: " + settings->getClientStrId(settings->loadActiveClientId()) + "\n");
    logger->append("SettingsDialog", "\tVersion: " + settings->loadClientVersion() + "\n");
    logger->append("SettingsDialog", "\tUseClientJava: " + QString(ui->javapathBox->isChecked() ? "true" : "false") + "\n");
    logger->append("SettingsDialog", "\tClientJava: " + ui->javapathEdit->text() + "\n");
    logger->append("SettingsDialog", "\tUseClientArgs: " + QString(ui->argsBox->isChecked() ? "true" : "false") + "\n");
    logger->append("SettingsDialog", "\tClientArgs: " + ui->argsEdit->text() + "\n");
    logger->append("SettingsDialog", "\tMinecraftGeometry: " +
                   QString::number(ui->widthSpinBox->value())  + "," +
                   QString::number(ui->heightSpinBox->value()) + "\n");
    logger->append("SettingsDialog", "\tFullscreen: " + QString(ui->fullscreenRadio->isChecked() ? "true" : "false")+"\n");
    logger->append("SettingsDialog", "\tUseLauncherSize: " + QString(ui->useLauncherRadio->isChecked() ? "true" : "false")+"\n");
}

void SettingsDialog::openFileDialog() {
    QString javapath = QFileDialog::getOpenFileName(this, "Выберите исполняемый файл java", "", "");
    ui->javapathEdit->setText(javapath);
}

void SettingsDialog::openClientDirectory() {

    QFile* clientDir = new QFile(settings->getClientDir());
    if (clientDir->exists()) {
        QUrl clientDirUrl = QUrl::fromLocalFile(clientDir->fileName());
        QDesktopServices::openUrl(clientDirUrl);
    } else {
        QMessageBox::critical(this, "У нас проблема :(", "Директория ещё не существует.\n"
                              + clientDir->fileName());
        logger->append("SettingsDialog", "Error: can't open client directory (not exists)\n");
    }
    delete clientDir;
}
