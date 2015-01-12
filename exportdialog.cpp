#include "exportdialog.h"
#include "ui_exportdialog.h"

#include <QFileDialog>
#include "util.h"

ExportDialog::ExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportDialog)
{
    ui->setupUi(this);
    settings = Settings::instance();
    logger = Logger::logger();

    logger->append("ExportDialog", "Export dialog opened\n");

    ui->clientCombo->addItems(settings->getClientsNames());
    ui->dirEdit->setText(QDir::homePath());

    connect(ui->clientCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(makeVersionList()));
    makeVersionList();

    connect(ui->exportButton, SIGNAL(clicked()), this, SLOT(makeExport()));
    connect(ui->dirlButton, SIGNAL(clicked()), this, SLOT(openDirDialog()));
}

ExportDialog::~ExportDialog() {
    logger->append("ExportDialog", "Export dialog closed\n");
    delete ui;
}

bool ExportDialog::copyFile(QString from, QString to) {

    QFileInfo toInfo(to);
    QDir(toInfo.absoluteDir().absolutePath()).mkpath(toInfo.absoluteDir().absolutePath());

    if (toInfo.exists()) {

        ui->log->appendPlainText("Файл существует: " + to);
        logger->append("ExportDialog", "File exists: " + to + "\n");
        return true;
    }

    bool result = QFile::copy(from, to);

    if (result) {

        ui->log->appendPlainText("Копирование: " + to);
        logger->append("ExportDialog", "Copy: " + to + "\n");

    } else {

        ui->log->appendPlainText("[!] Ошибка: " + to);
        logger->append("ExportDialog", "Error: " + to + "\n");

    }

    return result;
}

void ExportDialog::openDirDialog() {
    QString path = QFileDialog::getExistingDirectory(this, "Выберите директорию для экспорта", ui->dirEdit->text());
    if (!path.isEmpty()) {
        ui->dirEdit->setText(path);
    }
}

void ExportDialog::makeVersionList() {
    // Remove old entries
    ui->versionCombo->clear();

    if (ui->clientCombo->count() > 0) {
        // make entries list
        QDir verDir = QDir(settings->getBaseDir() + "/client_"
                         + settings->getClientStrId(ui->clientCombo->currentIndex())
                         + "/versions");
        ui->versionCombo->addItems(verDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    } else {
        ui->log->appendPlainText("ВНИМАНИЕ: Пустой список клиентов");
        logger->append("ExportDialog", "WARN: Empty client list!\n");
    }
}

void ExportDialog::makeExport() {

    ui->log->clear();
    ui->log->appendPlainText("Начинаю экспорт клиента...");
    logger->append("ExportDialog", "Begin export...\n");

    // Check for correct input data
    if (ui->clientCombo->count() < 1) {
        ui->log->appendPlainText("Ошибка: пустой список клиентов!");
        logger->append("ExportDialog", "Error: empty client list!\n");
        return;
    }

    if (ui->versionCombo->count() < 1) {
        ui->log->appendPlainText("Ошибка: пустой список версий!");
        logger->append("ExportDialog", "Error: empty version list!\n");
        return;
    }

    if (ui->dirEdit->text().isEmpty()) {
        ui->log->appendPlainText("Ошибка: не указана директория для экспорта!");
        logger->append("ExportDialog", "Error: export directory is not set!\n");
        return;
    }

    ui->clientCombo->setEnabled(false);
    ui->versionCombo->setEnabled(false);
    ui->dirEdit->setEnabled(false);
    ui->dirlButton->setEnabled(false);
    ui->exportButton->setEnabled(false);

    // Export assets
    ui->log->appendPlainText(" >> Экспорт: ассеты");
    logger->append("ExportDialog", "Section: assets\n");

    QString ver = ui->versionCombo->currentText();
    QFile versionFile(settings->getBaseDir() + "/client_"
                      + settings->getClientStrId(ui->clientCombo->currentIndex())
                      + "/versions/" + ver + "/" + ver + ".json");

    if (!versionFile.open(QIODevice::ReadOnly)) {

        ui->log->appendPlainText("Ошибка: не удалось открыть индекс <ver>.json!");
        logger->append("ExportDialog", "Error: cant open version json!\n");

    } else {

        QJsonDocument versionJDoc;
        QJsonParseError error;

        versionJDoc = QJsonDocument::fromJson(versionFile.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {

            ui->log->appendPlainText("Ошибка: не удалось разобрать индекс <ver>.json!");
            logger->append("ExportDialog", "Error: cant parse version json!\n");

        } else {

            QString assetsver = versionJDoc.object()["assets"].toString();
            QFile assetsIndex(settings->getBaseDir() + "/assets/indexes/" + assetsver + ".json");

            if (!assetsIndex.open(QIODevice::ReadOnly)) {

                ui->log->appendPlainText("Ошибка: не удалось открыть индекс ассетов!");
                logger->append("ExportDialog", "Error: cant open assets index json!\n");

            } else {

                QJsonDocument assetsDoc = QJsonDocument::fromJson(assetsIndex.readAll(), &error);
                if (error.error != QJsonParseError::NoError) {

                    ui->log->appendPlainText("Ошибка: не удалось разобрать индекс ассетов!");
                    logger->append("ExportDialog", "Error: cant parse assets index json!\n");

                } else {

                    // Copy assets index file
                    copyFile(settings->getBaseDir() + "/assets/indexes/" + assetsver + ".json",
                             ui->dirEdit->text()    + "/assets/indexes/" + assetsver + ".json");

                    // Copy assets
                    QJsonObject assets = assetsDoc.object()["objects"].toObject();
                    foreach (QString key, assets.keys()) {

                        QString hash = assets[key].toObject()["hash"].toString();
                        QString hashDir = hash.mid(0, 2);

                        copyFile(settings->getBaseDir() + "/assets/objects/" + hashDir + "/" + hash,
                                 ui->dirEdit->text()    + "/assets/objects/" + hashDir + "/" + hash);
                        QApplication::processEvents();
                    }

                }

            }

        }

    }

    // Export libraries
    ui->log->appendPlainText(" >> Экспорт: библиотеки");
    logger->append("ExportDialog", "Section: libraries\n");

    QFile dataFile(settings->getBaseDir() + "/client_"
                   + settings->getClientStrId(ui->clientCombo->currentIndex())
                   + "/versions/" + ver + "/" + "data.json");

    if (!dataFile.open(QIODevice::ReadOnly)) {

        ui->log->appendPlainText("Ошибка: не удалось открыть data.json!");
        logger->append("ExportDialog", "Error: cant open data.json!\n");

    } else {

        QJsonDocument dataDoc;
        QJsonParseError error;

        dataDoc = QJsonDocument::fromJson(dataFile.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {

            ui->log->appendPlainText("Ошибка: не удалось разобрать data.json!");
            logger->append("ExportDialog", "Error: cant parse data.json!\n");

        } else {

            QJsonObject libs = dataDoc.object()["libs"].toObject();
            foreach (QString suffix, libs.keys()) {
                copyFile(settings->getLibsDir() + "/" + suffix,
                         ui->dirEdit->text() + "/libraries/" + suffix);

                // Make libaray hash file
                QFile shaFile(ui->dirEdit->text() + "/libraries/" + suffix + ".sha1");
                if (!shaFile.exists()) {
                    if (shaFile.open(QIODevice::WriteOnly)) {
                        QByteArray hashData;
                        hashData.append(libs[suffix].toObject()["hash"].toString());
                        shaFile.write(hashData);
                        shaFile.close();
                    }
                }

                QApplication::processEvents();
            }

            // Export additional files
            ui->log->appendPlainText(" >> Экспорт: дополнительные файлы");
            logger->append("ExportDialog", "Section: additional files\n");

            QJsonObject files = dataDoc.object()["files"].toObject()["index"].toObject();
            foreach (QString suffix, files.keys()) {
                copyFile(settings->getBaseDir() + "/client_"
                         + settings->getClientStrId(ui->clientCombo->currentIndex())
                         + "/versions/" + ver + "/files/" + suffix,
                         ui->dirEdit->text() + "/"
                         + settings->getClientStrId(ui->clientCombo->currentIndex())
                         + "/" + ver + "/files/" + suffix);
                QApplication::processEvents();
            }

            QJsonArray mutables = dataDoc.object()["files"].toObject()["mutables"].toArray();
            QByteArray mutablesList;
            foreach (QJsonValue file, mutables) {
                mutablesList.append(file.toString() + '\n');
            }

            ui->log->appendPlainText("Генерация mutables.list");
            logger->append("ExportDialog", "Making mutables.list\n");

            QFile mutablesFile(ui->dirEdit->text() + "/"
                               + settings->getClientStrId(ui->clientCombo->currentIndex())
                               + "/" + ver + "/" + "mutables.list");

            // Create directory
            QFileInfo mutFinfo(mutablesFile);
            mutFinfo.absoluteDir().mkpath(mutFinfo.absoluteDir().path());

            if (mutablesFile.open(QIODevice::WriteOnly)) {
                mutablesFile.write(mutablesList);
                mutablesFile.close();
            } else {
                ui->log->appendPlainText("Ошибка: не удалось записать mutables.list!");
                logger->append("ExportDialog", "Error: cant write mutables.list!\n");
            }

        }

    }

    // Export main files (<ver>.jar, <ver>.json)
    ui->log->appendPlainText(" >> Экспорт: основные файлы");
    logger->append("ExportDialog", "Section: main files\n");

    copyFile(settings->getBaseDir() + "/client_"
             + settings->getClientStrId(ui->clientCombo->currentIndex())
             + "/versions/" + ver + "/" + ver + ".jar",
             ui->dirEdit->text() + "/"
             + settings->getClientStrId(ui->clientCombo->currentIndex())
             + "/" + ver + "/" + ver + ".jar");

    copyFile(settings->getBaseDir() + "/client_"
             + settings->getClientStrId(ui->clientCombo->currentIndex())
             + "/versions/" + ver + "/" + ver + ".json",
             ui->dirEdit->text() + "/"
             + settings->getClientStrId(ui->clientCombo->currentIndex())
             + "/" + ver + "/" + ver + ".json");

    ui->clientCombo->setEnabled(true);
    ui->versionCombo->setEnabled(true);
    ui->dirEdit->setEnabled(true);
    ui->dirlButton->setEnabled(true);
    ui->exportButton->setEnabled(true);
}
