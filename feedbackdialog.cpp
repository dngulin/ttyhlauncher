#include "feedbackdialog.h"
#include "ui_feedbackdialog.h"

#include "settings.h"
#include "util.h"
#include "jsonparser.h"

#include <QDebug>

FeedbackDialog::FeedbackDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FeedbackDialog)
{
    ui->setupUi(this);

    logger = Logger::logger();
    Settings *settings = Settings::instance();

    ui->nickEdit->setText( settings->loadLogin() );

    if ( settings->loadPassStoreState() )
    {
        ui->passEdit->setText( settings->loadPassword() );
    }

    connect(ui->sendButton, &QPushButton::clicked, this,
            &FeedbackDialog::sendFeedback);

    connect(&uploader, &DataFetcher::finished, this,
            &FeedbackDialog::requestFinished);
}

FeedbackDialog::~FeedbackDialog()
{
    delete ui;
}

void FeedbackDialog::log(const QString &text)
{
    logger->appendLine(tr("FeedBackDialog"), text);
}

void FeedbackDialog::msg(const QString &text)
{
    ui->messageLabel->setText(text);
    log(text);
}

void FeedbackDialog::sendFeedback()
{
    msg( tr("Preparing diagnostic log...") );

    QString brief = ui->descEdit->toPlainText();
    log(tr("Brief: ") + brief);

    QJsonObject payload;
    payload["username"] = ui->nickEdit->text();
    payload["password"] = ui->passEdit->text();

    payload["desc"] = "GZIP DATA";

    if ( ui->nickEdit->text().isEmpty() )
    {
        msg( tr("Error: nickname does not set!") );
        return;
    }

    if ( ui->passEdit->text().isEmpty() )
    {
        msg( tr("Error: password does not set!") );
        return;
    }

    ui->sendButton->setEnabled(false);

    Settings *settings = Settings::instance();

    QByteArray log;
    log.append("[General]\n");
    log.append("\n");

    log.append(settings->getOsName() + ", ");
    log.append(settings->getOsVersion() + ", ");
    log.append("arch: " + settings->getWordSize() + ".\n");
    log.append("\n");

    log.append("[Description]\n");
    log.append("\n");

    log.append(brief + "\n");
    log.append("\n");

    log.append("[Java]\n");
    log.append("\n");

    log.append( Util::getCommandOutput("java", QStringList() << "-version") );
    log.append("\n");

    log.append("[Logs]\n");
    log.append("\n");

    QString logsPrefix = settings->getBaseDir() + "/";
    for (int i = 0; i < 3; i++)
    {
        QString file = "launcher." + QString::number(i) + ".log";
        log.append("Log file \"" + file + "\":\n");
        log.append( Util::getFileContetnts(logsPrefix + file) );
        log.append("\n");
    }

    // Gzip and base-64 encode log
    QByteArray zlog = Util::makeGzip(log);
    payload["log"] = QString( zlog.toBase64() );

    QJsonDocument jsonRequest(payload);

    msg( tr("Uploading diagnostic log...") );
    uploader.makePost( Settings::feedbackUrl, jsonRequest.toJson() );
}

void FeedbackDialog::requestFinished(bool result)
{
    ui->sendButton->setEnabled(true);

    if (!result)
    {
        msg( tr("Error: ") + uploader.errorString() );
        return;
    }

    JsonParser parser;

    if ( !parser.setJson( uploader.getData() ) )
    {
        msg( tr("Bad server answer: ") + parser.getParserError() );
        return;
    }

    if ( parser.hasServerResponseError() )
    {
        msg( tr("Error: ") + parser.getServerResponseError() );
        return;
    }

    msg( tr("Feedback log successfully uploaded!") );
}
