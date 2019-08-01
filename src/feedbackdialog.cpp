#include "feedbackdialog.h"
#include "../ui/ui_feedbackdialog.h"

#include "oldsettings.h"
#include "util.h"
#include "jsonparser.h"

#include <QMessageBox>

FeedbackDialog::FeedbackDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FeedbackDialog)
{
    ui->setupUi(this);

    logger = OldLogger::logger();
    OldSettings *settings = OldSettings::instance();

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
    log( tr("Brief:\n%1").arg(brief) );

    QJsonObject payload;
    payload["username"] = ui->nickEdit->text();
    payload["password"] = ui->passEdit->text();

    payload["desc"] = "GZIP DATA";

    if ( ui->nickEdit->text().isEmpty() )
    {
        msg( tr("Error! Nickname does not set!") );
        return;
    }

    if ( ui->passEdit->text().isEmpty() )
    {
        msg( tr("Error! Password does not set!") );
        return;
    }

    ui->sendButton->setEnabled(false);

    OldSettings *settings = OldSettings::instance();

    QByteArray diag;
    diag.append("[General]\n");
    diag.append("\n");

    diag.append(settings->getOsName() + ", ");
    diag.append(settings->getOsVersion() + ", ");
    diag.append("arch: " + settings->getWordSize() + ".\n");
    diag.append("\n");

    diag.append("[Description]\n");
    diag.append("\n");

    diag.append(brief + "\n");
    diag.append("\n");

    diag.append("[Java]\n");
    diag.append("\n");

    diag.append( Util::getCommandOutput("java", QStringList() << "-version") );
    diag.append("\n");

    diag.append("[Logs]\n");
    diag.append("\n");

    QString logsPrefix = settings->getBaseDir() + "/";
    for (int i = 0; i < 3; i++)
    {
        QString file = QString("launcher.%1.log").arg(i);
        diag.append( QString("Log file '%1':\n").arg(file) );
        diag.append( Util::getFileContetnts(logsPrefix + file) );
        diag.append("\n");
    }

    // Gzip and base-64 encode log
    QByteArray zlog = Util::makeGzip(diag);
    payload["log"] = QString( zlog.toBase64() );

    QJsonDocument jsonRequest(payload);

    msg( tr("Uploading diagnostic log...") );
    uploader.makePost( OldSettings::feedbackUrl, jsonRequest.toJson() );
}

void FeedbackDialog::requestFinished(bool result)
{
    ui->sendButton->setEnabled(true);

    QString error = tr("Error! %1");

    if (!result)
    {
        msg( error.arg( uploader.errorString() ) );
        return;
    }

    JsonParser parser;

    if ( !parser.setJson( uploader.getData() ) )
    {
        msg( error.arg( parser.getParserError() ) );
        return;
    }

    if ( parser.hasServerResponseError() )
    {
        msg( error.arg( parser.getServerResponseError() ) );
        return;
    }

    QString title = tr("Complete!");
    QString message = tr("Feedback log successfully uploaded!");

    msg(message);
    QMessageBox::information(this, title, message);

    this->close();
}
