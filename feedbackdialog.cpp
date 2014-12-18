#include "feedbackdialog.h"
#include "ui_feedbackdialog.h"

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

    // Users input and logs trnslated to Base64

    //QByteArray desc; desc.append(ui->descEdit->toPlainText());
    //payload["desc"] = QString(desc.toBase64());
    payload["desc"] = "GZIP DATA";

    logger->append("FeedBackDialog", "Prepare diagnostic log...\n");
    Settings* settings = Settings::instance();

    QByteArray log;    
    log.append("## ============================= ##\n");
    log.append("    TTYHLAUNCHER DIAGNOSTIC LOG\n");
    log.append("## ============================= ##\n");
    log.append("\n");

    log.append(settings->getOsName() + ", ");
    log.append(settings->getOsVersion() + ", ");
    log.append("arch: " + settings->getWordSize() + ".\n");
    log.append("\n");

    log.append("## ============================= ##\n");
    log.append("        TROUBLE DESCRIPTION\n");
    log.append("## ============================= ##\n");
    log.append("\n");

    log.append(ui->descEdit->toPlainText() + "\n");
    log.append("\n");

    log.append("## ============================= ##\n");
    log.append("             JAVA INFO\n");
    log.append("## ============================= ##\n");
    log.append("\n");

    log.append(Util::getCommandOutput("java", QStringList() << "-version"));
    log.append("\n");

    // Show custom java -version if exists
    int activeClientId = settings->loadActiveClientId();
    QStringList clientList = settings->getClientsNames();
    foreach (QString client, clientList) {
        settings->saveActiveClientId(clientList.indexOf(client));

        if (settings->loadClientJavaState()) {
            log.append(" >> Client \"" + settings->getClientStrId(clientList.indexOf(client)) + "\" has custom java:\n");
            log.append(Util::getCommandOutput(settings->loadClientJava(), QStringList() << "-version"));
            log.append("\n");
        }

    }
    settings->saveActiveClientId(activeClientId);

    // Show logs
    log.append("## ============================= ##\n");
    log.append("             LOGS INFO\n");
    log.append("## ============================= ##\n");
    log.append("\n");

    QString logsPrefix = settings->getBaseDir() + "/";
    for (int i = 0; i < 3; i++) {
        log.append(" >> Log file \"launcher." + QString::number(i) + ".log\":\n");
        log.append(Util::getFileContetnts(logsPrefix + "launcher." + QString::number(i) + ".log"));
        log.append("\n");
    }

    // Gzip and base-64 encode log
    QByteArray zlog = Util::makeGzip(log);
    payload["log"] = QString(zlog.toBase64());

    QJsonDocument jsonRequest(payload);

    logger->append("FeedBackDialog", "Making request...\n");
    Reply serverReply = Util::makePost(Settings::feedbackUrl + ".gz", jsonRequest.toJson());

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
