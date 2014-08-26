#include "feedbackdialog.h"
#include "ui_feedbackdialog.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QIODevice>
#include <QFileDialog>

#include "settings.h"
#include "reply.h"
#include "util.h"

FeedbackDialog::FeedbackDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FeedbackDialog)
{
    ui->setupUi(this);

    logger = Logger::logger();

    Settings* settings = Settings::instance();
    ui->nickEdit->setText(settings->loadLogin());

    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendFeedback()));
    logger->append("FeedBackDialog", "Feedback dialog dialog opened\n");
}

FeedbackDialog::~FeedbackDialog()
{
    logger->append("FeedBackDialog", "Feedback dialog dialog closed\n");
    delete ui;
}

void FeedbackDialog::sendFeedback() {

    ui->sendButton->setEnabled(false);
    logger->append("FeedBackDialog", "Sending feedback, description:\n");
    logger->append("FeedBackDialog", "\"" + ui->descEdit->toPlainText() + "\"\n");

    QJsonObject payload;
    payload["username"] = ui->nickEdit->text();
    payload["password"] = ui->passEdit->text();
    payload["desc"] = ui->descEdit->toPlainText();  // FIXME (unsafe, base64?)
    payload["log"] = "diagnostic log will be here"; // FIXME

    QJsonDocument jsonRequest(payload);

    logger->append("FeedBackDialog", "Making request...\n");
    Reply serverReply = Util::makePost(Settings::feedbackUrl, jsonRequest.toJson());

    if (!serverReply.isOK()) {

        ui->messageLabel->setText("Ошибка: " + serverReply.getErrorString());
        logger->append("FeedBackDialog", "Error: " + serverReply.getErrorString() + "\n");

    } else {

        logger->append("FeedBackDialog", "OK\n");

        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(serverReply.reply(), &error);

        if (error.error == QJsonParseError::NoError) {

            QJsonObject responce = json.object();

            if (responce["error"].isNull()) {

                ui->messageLabel->setText("Сообщение об ошибке доставлено!");
                logger->append("FeedBackDialog", "Feedback sended\n");

            } else {
                // Error answer handler
                ui->messageLabel->setText("Ошибка: " + responce["error"].toString());
                logger->append("FeedBackDialog", "Error:"
                                         + responce["error"].toString() + "\n");
            }

        } else {
            // JSON parse error
            ui->messageLabel->setText("Ошибка: сервер ответил ерунду...");
            logger->append("FeedBackDialog", "JSON parse error!\n");
        }
    }

    ui->sendButton->setEnabled(true);
}
