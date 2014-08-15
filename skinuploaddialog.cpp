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

    Settings* settings = Settings::instance();
    ui->nickEdit->setText(settings->loadLogin());

    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(uploadSkin()));
    connect(ui->browseButton, SIGNAL(clicked()), this, SLOT(openFileDialog()));
}

SkinUploadDialog::~SkinUploadDialog()
{
    delete ui;
}

void SkinUploadDialog::uploadSkin() {

    QFile* skinfile = new QFile(ui->pathEdit->text());
    if (!skinfile->exists()) {
        ui->messageLabel->setText("Ошибка: файл скина не существует :(");
        return;
    }

    if (!skinfile->open(QIODevice::ReadOnly)) {
        ui->messageLabel->setText("Ошибка: не удалось открыть файл скина :(");
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

    qDebug() << data.toJson();

    QByteArray postdata;
    postdata.append(data.toJson());

    request.setUrl(Settings::skinUploadUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, postdata.size());

    QNetworkReply *reply = manager->post(request, postdata);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

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
                ui->messageLabel->setText("Ошибка: " + responce["error"].toString());

            } else {
                // Correct request
                ui->messageLabel->setText("Поздравляем! Скин успешно изменён!");
            }

        } else {
            // JSON parse error
            ui->messageLabel->setText("Ошибка: сервер ответил ерунду...");
        }

    } else {
        // Connection error
        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            ui->messageLabel->setText("Ошибка: неправильный логин или пароль!");
        } else {
            ui->messageLabel->setText("Ошибка: " + reply->errorString());
        }

    }

   delete manager;
   delete reply;

    ui->sendButton->setEnabled(true);
}

void SkinUploadDialog::openFileDialog() {
    QString path = QFileDialog::getOpenFileName(this, "Выберите файл скина", QDir::homePath(), "*.png");
    ui->pathEdit->setText(path);
}

