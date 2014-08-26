#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QFileDialog>
#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>

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
    emit ui->clientCombo->activated(ui->clientCombo->currentIndex());

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
    ui->versionCombo->addItem("Последняя доступня версия", "latest");

    QNetworkRequest request;
    // FIXME: in release url depended at activeClient value
    request.setUrl(QUrl("https://s3.amazonaws.com/Minecraft.Download/versions/versions.json"));
    logger->append("SettingsDialog", "Making version list request...\n");
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
                ui->stateEdit->setText("Список версий с сервера обновлений");
                logger->append("SettingsDialog", "List downloaded\n");

                QJsonArray versions = responce["versions"].toArray();
                QJsonObject version;
                for (QJsonArray::iterator it = versions.begin(), end = versions.end(); it != end; ++it) {
                    version = (*it).toObject();
                    if (version["type"].toString() == "release")
                        ui->versionCombo->addItem(version["id"].toString(), version["id"].toString());
                }

                if (ui->versionCombo->count() > 1) {
                    QString strVerId = settings->loadClientVersion();
                    for (int i = 0; i <= ui->versionCombo->count(); i++) {
                        if (ui->versionCombo->itemData(i).toString() == strVerId) {
                            ui->versionCombo->setCurrentIndex(i);
                            break;
                        }
                    }
                    ui->versionCombo->setEnabled(true);
                }
            }

        } else {
            // JSON parse error
            logger->append("SettingsDialog", "JSON parse error!\n");
            makeLocalVersionList("Локальные версии (ошибка обмена с севрером)");
        }


    } else {
        // Connection error
        logger->append("SettingsDialog", "Connection error!\n");
        makeLocalVersionList("Локальные версии (сервер недоступен)");
    }

}

void SettingsDialog::makeLocalVersionList(QString reason) {
    logger->append("SettingsDialog", "Making local version list...\n");
    ui->stateEdit->setText(reason);

    QString prefix = settings->getClientDir() + "/versions";
    QDir verdir = QDir(prefix);
    QStringList subdirs = verdir.entryList();
    for (QStringList::iterator nameit = subdirs.begin(), end = subdirs.end(); nameit != end; ++nameit) {
        QString ver = (*nameit);
        QFile* file = new QFile(prefix + "/" + ver + "/" + ver + ".json");
        if (file->exists()) {
            ui->versionCombo->addItem(ver, ver);
        }
        delete file;
    }
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

    logger->append("SettingsDialog", "Settings saved\n");
    logger->append("SettingsDialog", "\tClient: " + settings->getClientStrId(settings->loadActiveClientId()) + "\n");
    logger->append("SettingsDialog", "\tVersion: " + settings->loadClientVersion() + "\n");
    logger->append("SettingsDialog", "\tUseClientJava: " + QString(ui->javapathBox->isChecked() ? "true" : "false") + "\n");
    logger->append("SettingsDialog", "\tClientJava: " + ui->javapathEdit->text() + "\n");
    logger->append("SettingsDialog", "\tUseClientArgs: " + QString(ui->argsBox->isChecked() ? "true" : "false") + "\n");
    logger->append("SettingsDialog", "\tClientArgs: " + ui->argsEdit->text() + "\n");

}

void SettingsDialog::loadSettings() {

    // Setup settings
    ui->javapathBox->setChecked(settings->loadClientJavaState());
    ui->javapathEdit->setText(settings->loadClientJava());
    ui->argsBox->setChecked(settings->loadClientJavaArgsState());
    ui->argsEdit->setText(settings->loadClientJavaArgs());

    logger->append("SettingsDialog", "Settings loaded\n");
    logger->append("SettingsDialog", "\tClient: " + settings->getClientStrId(settings->loadActiveClientId()) + "\n");
    logger->append("SettingsDialog", "\tVersion: " + settings->loadClientVersion() + "\n");
    logger->append("SettingsDialog", "\tUseClientJava: " + QString(ui->javapathBox->isChecked() ? "true" : "false") + "\n");
    logger->append("SettingsDialog", "\tClientJava: " + ui->javapathEdit->text() + "\n");
    logger->append("SettingsDialog", "\tUseClientArgs: " + QString(ui->argsBox->isChecked() ? "true" : "false") + "\n");
    logger->append("SettingsDialog", "\tClientArgs: " + ui->argsEdit->text() + "\n");
}

void SettingsDialog::openFileDialog() {
    QString javapath = QFileDialog::getOpenFileName(this, "Выберите исполняемый файл java", "", "");
    ui->javapathEdit->setText(javapath);
}

void SettingsDialog::openClientDirectory() {

    QFile* clientDir = new QFile(settings->getClientDir());
    if (clientDir->exists()) {
        QUrl clientDirUrl = QUrl(clientDir->fileName());
        QDesktopServices::openUrl(clientDirUrl);
    } else {
        QMessageBox::critical(this, "У нас проблема :(", "Директория ещё не существует.\n"
                              + clientDir->fileName());
        logger->append("SettingsDialog", "Error: can't open client directory (not exists)\n");
    }
    delete clientDir;
}
