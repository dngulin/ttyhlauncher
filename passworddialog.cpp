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

    logger = Logger::logger();

    Settings* settings = Settings::instance();
    ui->nickEdit->setText(settings->loadLogin());
    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(changePassword()));

    logger->append("PasswordDialog", "Password dialog opened\n");
}

PasswordDialog::~PasswordDialog()
{
    logger->append("PasswordDialog", "Password dialog closed\n");
    delete ui;
}

void PasswordDialog::changePassword() {

    logger->append("PasswordDialog", "Try to change password...\n");

    if (ui->newpassEdit->text() != ui->confirmEdit->text()) {
        ui->messageLabel->setText("Ошибка: пароли не совпадают :(");
        logger->append("PasswordDialog", "Error: not equal passwords\n");
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

    logger->append("PasswordDialog", "Making network request...\n");
    QNetworkReply *reply = manager->post(request, postdata);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // Check for connection error
    if (reply->error() == QNetworkReply::NoError) {
        logger->append("PasswordDialog", "OK\n");

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
                logger->append("PasswordDialog", "Error: "
                               + responce["error"].toString() + "\n");

            } else {
                // Correct request
                ui->messageLabel->setText("Пароль успешно изменён!");
                logger->append("PasswordDialog", "Password changed!\n");
            }

        } else {
            // JSON parse error
            ui->messageLabel->setText("Ошибка: сервер ответил ерунду...");
            logger->append("PasswordDialog", "JSON parsing error\n");
        }

    } else {
        // Connection error
        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            ui->messageLabel->setText("Ошибка: неправильный логин или пароль!");
            logger->append("PasswordDialog", "Error: bad login\n");
        } else {
            ui->messageLabel->setText("Ошибка: " + reply->errorString());
            logger->append("PasswordDialog", "Error: " + reply->errorString() + "\n");
        }

    }

    delete manager;

    ui->sendButton->setEnabled(true);
}
