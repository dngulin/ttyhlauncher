#include "passworddialog.h"
#include "ui_passworddialog.h"

#include "settings.h"
#include "util.h"
#include "reply.h"

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

    // Make JSON login request
    QJsonObject payload;
    payload["username"] = ui->nickEdit->text();
    payload["password"] = ui->passEdit->text();
    payload["newpassword"] = ui->newpassEdit->text();

    QJsonDocument jsonRequest(payload);

    logger->append("PasswordDialog", "Making request...\n");
    Reply serverReply = Util::makePost(Settings::changePasswrdUrl, jsonRequest.toJson());

    if (!serverReply.isOK()) {

        ui->messageLabel->setText("Ошибка: " + serverReply.getErrorString());
        logger->append("PasswordDialog", "Error: " + serverReply.getErrorString() + "\n");

    } else {

        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(serverReply.reply(), &error);

        // Check for incorrect JSON
        if (error.error == QJsonParseError::NoError) {

            QJsonObject responce = json.object();

            // Check for error in server answer
            if (!responce["error"].isNull()) {

                ui->messageLabel->setText("Ошибка: " + responce["error"].toString());
                logger->append("PasswordDialog", "Error: "  + responce["error"].toString() + "\n");

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
    }

    ui->sendButton->setEnabled(true);
}
