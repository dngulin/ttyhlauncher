#include "skinuploaddialog.h"
#include "ui_skinuploaddialog.h"

#include "settings.h"
#include "util.h"
#include "reply.h"

#include <QFileDialog>

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

    // Check for skin image size
    QImage* skinImage = new QImage(ui->pathEdit->text());
    int imageHeight = skinImage->height();
    int imageWidth = skinImage->width();
    delete skinImage;

    if ((imageHeight != 32 && imageHeight !=64) || imageWidth != 64) {
        ui->messageLabel->setText("Ошибка: скин имеет неверное разрешение :(");
        logger->append("SkinUploadDialog", "Error: skin file has incorrect resolution\n");
        return;
    }

    if (ui->nickEdit->text().isEmpty()) {
        ui->messageLabel->setText("Ошибка: игровое имя не может быть пустым");
        logger->append("SkinUploadDialog", "Error: empty nickname\n");
        return;
    }
    if (ui->passEdit->text().isEmpty()) {
        ui->messageLabel->setText("Ошибка: пароль не может быть пустым");
        logger->append("SkinUploadDialog", "Error: empty password\n");
        return;
    }

    ui->sendButton->setEnabled(false);

    // Make JSON login request
    QJsonObject payload;
    payload["username"] = ui->nickEdit->text();
    payload["password"] = ui->passEdit->text();

    QByteArray skin(skinfile->readAll());
    payload["skinData"] = QString(skin.toBase64());

    QJsonDocument jsonRequest(payload);

    logger->append("SkinUploadDialog", "Making request...\n");
    Reply serverReply = Util::makePost(Settings::skinUploadUrl, jsonRequest.toJson());

    if (!serverReply.isSuccess()) {

        ui->messageLabel->setText("Ошибка: " + serverReply.getErrorString());
        logger->append("SkinUploadDialog", "Error: " + serverReply.getErrorString() + "\n");

    } else {

        logger->append("SkinUploadDialog", "OK\n");
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(serverReply.getData(), &error);

        // Check for incorrect JSON
        if (error.error == QJsonParseError::NoError) {

            QJsonObject responce = json.object();

            if (!responce["error"].isNull()) {

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
    }

    ui->sendButton->setEnabled(true);
}

void SkinUploadDialog::openFileDialog() {
    QString path = QFileDialog::getOpenFileName(this, "Выберите файл скина", QDir::homePath(), "*.png");
    ui->pathEdit->setText(path);
    logger->append("SkinUploadDialog", "Selected skin-file: " + path + "\n");
}

