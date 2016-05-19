#include "skinuploaddialog.h"
#include "ui_skinuploaddialog.h"

#include "settings.h"
#include "jsonparser.h"

#include <QFileDialog>
#include <QMessageBox>

SkinUploadDialog::SkinUploadDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SkinUploadDialog)
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
            &SkinUploadDialog::uploadSkin);
    connect(ui->browseButton, &QPushButton::clicked, this,
            &SkinUploadDialog::openFileDialog);

    connect(&uploader, &DataFetcher::finished, this,
            &SkinUploadDialog::requestFinished);
}

SkinUploadDialog::~SkinUploadDialog()
{
    delete ui;
}

void SkinUploadDialog::log(const QString &text)
{
    logger->appendLine(tr("SkinUploadDialog"), text);
}

void SkinUploadDialog::msg(const QString &text)
{
    ui->messageLabel->setText(text);
    log(text);
}

void SkinUploadDialog::uploadSkin()
{
    msg( tr("Uploading...") );

    // Check input data
    QFile skinfile( ui->pathEdit->text() );
    if ( !skinfile.exists() )
    {
        msg( tr("Error! Skin file does not exists!") );
        return;
    }

    if ( !skinfile.open(QIODevice::ReadOnly) )
    {
        msg( tr("Error! Can't open skin file!") );
        return;
    }

    QImage skinImage( ui->pathEdit->text() );
    int height = skinImage.height();
    int width = skinImage.width();

    if ( (height != 32 && height != 64) || width != 64 )
    {
        msg( tr("Error! Skin image has wrong resolution!") );
        return;
    }

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

    // Make request
    QJsonObject payload;
    payload["username"] = ui->nickEdit->text();
    payload["password"] = ui->passEdit->text();

    QByteArray skin( skinfile.readAll() );
    payload["skinData"] = QString( skin.toBase64() );

    QJsonDocument jsonRequest(payload);

    uploader.makePost( Settings::skinUploadUrl, jsonRequest.toJson() );
    skinfile.close();
}

void SkinUploadDialog::requestFinished(bool result)
{
    ui->sendButton->setEnabled(true);

    if (!result)
    {
        msg( tr("Error! %1").arg( uploader.errorString() ) );
        return;
    }

    JsonParser parser;

    if ( !parser.setJson( uploader.getData() ) )
    {
        msg( tr("Bad server answer! %1").arg( parser.getParserError() ) );
        return;
    }

    if ( parser.hasServerResponseError() )
    {
        msg( tr("Error! %1").arg( parser.getServerResponseError() ) );
        return;
    }

    QString title = tr("Complete!");
    QString message = tr("Skin successfully uploaded!");

    msg(message);
    QMessageBox::information(this, title, message);

    this->close();
}

void SkinUploadDialog::openFileDialog()
{
    QString title = tr("Select a skin file");
    QString home = QDir::homePath();

    QString path = QFileDialog::getOpenFileName(this, title, home, "*.png");
    ui->pathEdit->setText(path);

    if ( !path.isEmpty() )
    {
        log( tr("File selected: %1").arg(path) );
    }
}
