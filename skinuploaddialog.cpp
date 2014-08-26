#include "skinuploaddialog.h"
#include "ui_skinuploaddialog.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QIODevice>
#include <QFileDialog>

#include "settings.h"

SkinUploadDialog::SkinUploadDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SkinUploadDialog)
{
    ui->setupUi(this);

    logger = Logger::logger();
    logger->append("SkinUploadDialog", "Skin upload dialog opened\n");

    Settings* settings = Settings::instance();
    ui->nickEdit->setText(settings->loadLogin());

    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(uploadSkin()));
    connect(ui->browseButton, SIGNAL(clicked()), this, SLOT(openFileDialog()));
}

SkinUploadDialog::~SkinUploadDialog()
{
    logger->append("SkinUploadDialog", "Skin upload dialog closed\n");
    delete ui;
}

void SkinUploadDialog::uploadSkin() {

    logger->append("SkinUploadDialog", "Try to upload skin\n");

    QFile* skinfile = new QFile(ui->pathEdit->text());
    if (!skinfile->exists()) {
        ui->messageLabel->setText("Ошибка: файл скина не существует :(");
        logger->append("SkinUploadDialog", "Error: skin file not exists\n");
        return;
    }

    if (!skinfile->open(QIODevice::ReadOnly)) {
        ui->messageLabel->setText("Ошибка: не удалось открыть файл скина :(");
        logger->append("SkinUploadDialog", "Error: can't open skinfile\n");
        return;
    }

    ui->sendButton->setEnabled(false);

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    // Make JSON login request
    QJsonDocument data;
    QJsonObject login;
    QNetworkRequest request;

    login["username"] = ui->nickEdit->text();
    login["password"] = ui->passEdit->text();

    QByteArray skin;
    skin.append(skinfile->readAll());
    login["skinData"] = QString(skin.toBase64());

    data.setObject(login);

    QByteArray postdata;
    postdata.append(data.toJson());

    request.setUrl(Settings::skinUploadUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, postdata.size());

    logger->append("SkinUploadDialog", "Making request...\n");
    QNetworkReply *reply = manager->post(request, postdata);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // Check for connection error
    if (reply->error() == QNetworkReply::NoError) {
        logger->append("SkinUploadDialog", "OK\n");

        QByteArray rawResponce = reply->readAll();
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(rawResponce, &error);

        // Check for incorrect JSON
        if (error.error == QJsonParseError::NoError) {

            QJsonObject responce = json.object();

            // Check for error in server answer
            if (!responce["error"].isNull()) {
                // Error in answer handler
                ui->messageLabel->setText("Ошибка: " + responce["error"].toString());
                logger->append("SkinUploadDialog", "Error: " + responce["error"].toString() + "\n");

            } else {
                // Correct request
                ui->messageLabel->setText("Поздравляем! Скин успешно изменён!");
                logger->append("SkinUploadDialog", "Skin changed!\n");
            }

        } else {
            // JSON parse error
            ui->messageLabel->setText("Ошибка: сервер ответил ерунду...");
            logger->append("SkinUploadDialog", "JSON parse error\n");
        }

    } else {
        // Connection error
        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            ui->messageLabel->setText("Ошибка: неправильный логин или пароль!");
            logger->append("SkinUploadDialog", "Error: bad login\n");
        } else {
            ui->messageLabel->setText("Ошибка: " + reply->errorString());
            logger->append("SkinUploadDialog", "Error: " + reply->errorString() + "\n");
        }

    }

   delete manager;

    ui->sendButton->setEnabled(true);
}

void SkinUploadDialog::openFileDialog() {
    QString path = QFileDialog::getOpenFileName(this, "Выберите файл скина", QDir::homePath(), "*.png");
    ui->pathEdit->setText(path);
    logger->append("SkinUploadDialog", "Selected skin-file: " + path + "\n");
}

