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

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    // Make JSON login request
    QJsonDocument data;
    QJsonObject login;
    QNetworkRequest request;

    login["username"] = ui->nickEdit->text();
    login["password"] = ui->passEdit->text();
    login["desc"] = ui->descEdit->toPlainText();
    login["log"] = "diagnostic log will be here"; // FIXME

    data.setObject(login);

    QByteArray postdata;
    postdata.append(data.toJson());

    request.setUrl(Settings::feedbackUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, postdata.size());

    QNetworkReply *reply = manager->post(request, postdata);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // Check for connection error
    if (reply->error() == QNetworkReply::NoError) {

        logger->append("FeedBackDialog", "OK\n");

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
                logger->append("FeedBackDialog", "Error:"
                                         + responce["error"].toString() + "\n");

            } else {
                // Correct request
                ui->messageLabel->setText("Сообщение об ошибке доставлено!");
                logger->append("FeedBackDialog", "Feedback sended\n");
            }

        } else {
            // JSON parse error
            ui->messageLabel->setText("Ошибка: сервер ответил ерунду...");
            logger->append("FeedBackDialog", "JSON parse error!\n");

        }

    } else {
        // Connection error
        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            ui->messageLabel->setText("Ошибка: неправильный логин или пароль!");
            logger->append("FeedBackDialog", "Error: bad login\n");
        } else {
            ui->messageLabel->setText("Ошибка: " + reply->errorString());
            logger->append("FeedBackDialog", "Error: " + reply->errorString() + "\n");
        }

    }

    delete manager;

    ui->sendButton->setEnabled(true);
}
