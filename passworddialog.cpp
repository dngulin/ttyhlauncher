#include "passworddialog.h"
#include "ui_passworddialog.h"
#include "settings.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>

PasswordDialog::PasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasswordDialog)
{
    ui->setupUi(this);

    Settings* settings = Settings::instance();
    ui->nickEdit->setText(settings->loadLogin());
    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(changePassword()));
}

PasswordDialog::~PasswordDialog()
{
    delete ui;
}

void PasswordDialog::changePassword() {

    if (ui->newpassEdit->text() != ui->confirmEdit->text()) {
        ui->messageLabel->setText("Ошибка: пароли не совпадают :(");
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
    login["newpassword"] = ui->newpassEdit->text();

    data.setObject(login);

    QByteArray postdata;
    postdata.append(data.toJson());

    request.setUrl(Settings::changePasswrdUrl);
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
                ui->messageLabel->setText("Пароль успешно изменён!");
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
