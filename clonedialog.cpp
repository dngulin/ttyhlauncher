#include "clonedialog.h"
#include "ui_clonedialog.h"

#include <QStringList>

#include "util.h"

CloneDialog::CloneDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CloneDialog)
{
    ui->setupUi(this);
    settings = Settings::instance();
    logger = Logger::logger();

    logger->append("CloneDialog", "Version clone dialog opened\n");

    loadVersionList();

    ui->clientCombo->addItems(settings->getClientsNames());

    connect(ui->cloneButton, SIGNAL(clicked()), this, SLOT(makeClone()));
}

CloneDialog::~CloneDialog() {
    delete ui;
}

bool CloneDialog::loadVersionList() {

    Reply reply = Util::makeGet("http://s3.amazonaws.com/Minecraft.Download/versions/versions.json");
    if (reply.isOK()) {
        QJsonParseError error;
        QJsonDocument vjson = QJsonDocument::fromJson(reply.reply(), &error);
        if (error.error == QJsonParseError::NoError) {
            QJsonArray vlist = vjson.object()["versions"].toArray();
            foreach (QJsonValue ver, vlist) {
                ui->sourceCombo->addItem(ver.toObject()["id"].toString());
            }
        } else {
            ui->log->appendPlainText("Не удалось разобрать список версий");
            logger->append("CloneDialog", "Error: can't parse version list\n");
        }
    } else {
        ui->log->appendPlainText("Не удалось получить список версий");
        logger->append("CloneDialog", "Error: can't get version list. " + reply.getErrorString() + "\n");
    }

    return true;
}

void CloneDialog::makeClone() {

    ui->log->appendPlainText("Начинаю клонирование версии...");
    logger->append("CloneDialog", "Begin version clone...\n");

    // Check for correct input values
    if (ui->sourceCombo->currentIndex() < 0) {
        ui->log->appendPlainText("Ошибка: Пустой список версий!");
        logger->append("CloneDialog", "Error: empty version list!\n");
        return;
    }

    if (ui->clientCombo->currentIndex() < 0) {
        ui->log->appendPlainText("Ошибка: Пустой список клиентов!");
        logger->append("CloneDialog", "Error: empty client list!\n");
        return;
    }

    if (ui->versionEdit->text() == "") {
        ui->log->appendPlainText("Ошибка: Не указано название версии для клонирования!");
        logger->append("CloneDialog", "Error: destination version not specified!\n");
        return;
    }

    // Disable inputs
    ui->sourceCombo->setEnabled(false);
    ui->clientCombo->setEnabled(false);
    ui->versionEdit->setEnabled(false);
    ui->cloneButton->setEnabled(false);

    // Prepare directory before download
    QString path = settings->getBaseDir() + "/client_"
            + settings->getClientStrId(ui->clientCombo->currentIndex())
            + "/versions/" + ui->versionEdit->text() + "/";
    QDir(path).mkpath(path);

    // Try to get files
    QStringList exts;
    exts << ".json" << ".jar";

    foreach (QString ext, exts) {
        ui->log->appendPlainText("Загрузка файла " + ui->sourceCombo->currentText() + ext + "...");
        QApplication::processEvents();
        Reply reply = Util::makeGet("http://s3.amazonaws.com/Minecraft.Download/versions/"
                                    + ui->sourceCombo->currentText() + "/"
                                    + ui->sourceCombo->currentText() + ext);
        if (reply.isOK()) {
            QFile file(path + ui->versionEdit->text() + ext);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply.reply());
                file.close();
            } else {
                // cant open file
                ui->log->appendPlainText("Ошибка: Не удалось сохранить файл " + ui->versionEdit->text() + ext);
                logger->append("CloneDialog", "Error: can't save file "  + ui->versionEdit->text() + ext
                               + ": " + reply.getErrorString() + "\n");
                return;
            }
        } else {
            // cant get file
            ui->log->appendPlainText("Ошибка: Не удалось получить файл " + ui->sourceCombo->currentText() + ext);
            logger->append("CloneDialog", "Error: can't get file "  + ui->sourceCombo->currentText() + ext
                           + ": " + reply.getErrorString() + "\n");
            return;
        }
    }

    // Edit downloaded JSON
    ui->log->appendPlainText("Редактирование id в " + ui->versionEdit->text() + ".json");
    logger->append("CloneDialog", "Changing id in "  + ui->versionEdit->text() + ".json\n");

    QFile versionFile(path + ui->versionEdit->text() + ".json");
    if (versionFile.exists()) {
        if (versionFile.open(QIODevice::ReadOnly)) {

            QJsonParseError error;
            QJsonDocument versionJson = QJsonDocument::fromJson(versionFile.readAll(), &error);
            versionFile.close();

            if ( error.error == QJsonParseError::NoError ) {

                QJsonObject jsonRoot = versionJson.object();
                jsonRoot["id"] = ui->versionEdit->text();

                if (versionFile.open(QIODevice::WriteOnly)) {

                    versionJson.setObject(jsonRoot);
                    versionFile.reset();
                    versionFile.write(versionJson.toJson());

                    versionFile.close();
                } else {
                    ui->log->appendPlainText("Ошибка: не удалось записать файл: " + ui->versionEdit->text() + ".json");
                    logger->append("CloneDialog", "Error: can't write file: "  + ui->versionEdit->text() + ".json\n");
                }
            } else {
                ui->log->appendPlainText("Ошибка: не удалось разобрать JSON: " + ui->versionEdit->text() + ".json");
                logger->append("CloneDialog", "Error: can't parse JSON: "  + ui->versionEdit->text() + ".json\n");
            }
        } else {
            ui->log->appendPlainText("Ошибка: не удалось открыть файл: " + ui->versionEdit->text() + ".json");
            logger->append("CloneDialog", "Error: can't open file: "  + ui->versionEdit->text() + ".json\n");
        }
    } else {
        ui->log->appendPlainText("Ошибка: файл не существует: " + ui->versionEdit->text() + ".json");
        logger->append("CloneDialog", "Error: file not exists: "  + ui->versionEdit->text() + ".json\n");
    }

    // Enable inputs
    ui->sourceCombo->setEnabled(true);
    ui->clientCombo->setEnabled(true);
    ui->versionEdit->setEnabled(true);
    ui->cloneButton->setEnabled(true);

    ui->log->appendPlainText("Версия успешно клонирована.");
    logger->append("CloneDialog", "Version clone finished.\n");
}
