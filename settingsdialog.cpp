#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "settings.h"

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

    Settings* settings = Settings::instance();
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

void SettingsDialog::loadVersionList() {
    ui->stateEdit->setText("Составляется список версий...");

    ui->versionCombo->setEnabled(false);
    ui->versionCombo->clear();
    ui->versionCombo->addItem("Последняя доступня версия", "latest");

    QNetworkRequest request;
    // FIXME: in release url depended at activeClient value
    request.setUrl(QUrl("https://s3.amazonaws.com/Minecraft.Download/versions/versions.json"));
    nam->get(request);
}

void SettingsDialog::makeVersionList(QNetworkReply* reply) {

    // Check for connection error
    if (reply->error() == QNetworkReply::NoError) {

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

            } else {
                // Correct login
                ui->stateEdit->setText("Список версий с сервера обновлений");

                QJsonArray versions = responce["versions"].toArray();
                QJsonObject version;
                for (QJsonArray::iterator it = versions.begin(), end = versions.end(); it != end; ++it) {
                    version = (*it).toObject();
                    if (version["type"].toString() == "release")
                        ui->versionCombo->addItem(version["id"].toString(), version["id"].toString());
                }

                if (ui->versionCombo->count() > 1) {
                    QString strVerId = Settings::instance()->loadClientVersion();
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
            makeLocalVersionList("Локальные версии (ошибка обмена с севрером)");
        }


    } else {
        // Connection error
        makeLocalVersionList("Локальные версии (сервер недоступен)");
    }

}

void SettingsDialog::makeLocalVersionList(QString reason) {
    ui->stateEdit->setText(reason);
}

void SettingsDialog::saveSettings() {
    Settings* settings = Settings::instance();

    int id = ui->versionCombo->currentIndex();
    QString strid = ui->versionCombo->itemData(id).toString();
    settings->saveClientVersion(strid);

    settings->saveClientJavaState(ui->javapathBox->isChecked());
    settings->saveClientJava(ui->javapathEdit->text());
    settings->saveClientJavaArgsState(ui->argsBox->isChecked());
    settings->saveClientJavaArgs(ui->argsEdit->text());
}

void SettingsDialog::loadSettings() {
    Settings* settings = Settings::instance();

    // Setup settings
    ui->javapathBox->setChecked(settings->loadClientJavaState());
    ui->javapathEdit->setText(settings->loadClientJava());
    ui->argsBox->setChecked(settings->loadClientJavaArgsState());
    ui->argsEdit->setText(settings->loadClientJavaArgs());
}

void SettingsDialog::openFileDialog() {
    QString javapath = QFileDialog::getOpenFileName(this, "Выберите исполняемый файл java", "", "");
    ui->javapathEdit->setText(javapath);
}

void SettingsDialog::openClientDirectory() {

    Settings* settings = Settings::instance();
    QFile* clientDir = new QFile(settings->getClientDir());
    if (clientDir->exists()) {
        QUrl clientDirUrl = QUrl(clientDir->fileName());
        QDesktopServices::openUrl(clientDirUrl);
    } else {
        QMessageBox::critical(this, "У нас проблема :(", "Директория ещё не существует.\n"
                              + clientDir->fileName());
    }
    delete clientDir;
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
