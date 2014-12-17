#include "fetchdialog.h"
#include "ui_fetchdialog.h"
#include "util.h"

FetchDialog::FetchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FetchDialog)
{
    ui->setupUi(this);
    settings = Settings::instance();
    logger = Logger::logger();

    connect(ui->fetchButton, SIGNAL(clicked()), this, SLOT(makeFetch()));

    ui->clientCombo->addItems(settings->getClientsNames());

    connect(ui->clientCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(makeVersionList()));
    makeVersionList();

}

FetchDialog::~FetchDialog() {
    delete ui;
}

void FetchDialog::downloadFile(QString url, QString fname) {

    ui->log->appendPlainText("Загрузка файла " + fname);
    logger->append("FetchDialog", "Downloading file " + fname + "\n");

    QFile file(fname);
    if (!file.exists()) {
        if (!Util::downloadFile(url, fname)) {
            ui->log->appendPlainText("Ошибка: не удалось загрузить файл");
            logger->append("FetchDialog", "Error can't get file\n");
            errList << QString("Не удалось загрузить: ") + fname.split('/').last();
        }
    } else {
        ui->log->appendPlainText("Пропуск: файл уже существует ");
        logger->append("FetchDialog", "Skipped: file exists\n");
    }
}

void FetchDialog::makeFetch() {

    if ( (ui->clientCombo->count() < 1) || (ui->versionCombo->count() < 1) ) {
        return;
    }

    ui->clientCombo->setEnabled(false);
    ui->versionCombo->setEnabled(false);
    ui->fetchButton->setEnabled(false);

    ui->log->clear();
    errList.clear();
    errList << "-----" << "Список ошибок возникших при полной загрузке файлов:";

    QString indexName = settings->getBaseDir()
                      + "/client_"
                      + settings->getClientStrId(ui->clientCombo->currentIndex())
                      + "/versions/" + ui->versionCombo->currentText() + "/"
                      + ui->versionCombo->currentText() + ".json";

    QFile indexFile(indexName);
    if (indexFile.open(QIODevice::ReadOnly)) {

        QJsonParseError error;
        QJsonDocument indexJDoc = QJsonDocument::fromJson(indexFile.readAll(), &error);

        if (error.error == QJsonParseError::NoError) {
            QJsonObject index = indexJDoc.object();
            QJsonArray libs = index["libraries"].toArray();

            // Fetch all libraries
            ui->log->appendPlainText("Загрузка библиотек... ");
            logger->append("FetchDialog", "Fetching libs... \n");

            foreach (QJsonValue lib, libs) {

                QApplication::processEvents();

                QString baseDir = settings->getLibsDir() + "/";
                QString baseUrl = "https://libraries.minecraft.net/";
                QString codedName = lib.toObject()["name"].toString();

                QString domen, name, version;
                domen =   codedName.split(':')[0];
                name =    codedName.split(':')[1];
                version = codedName.split(':')[2];

                domen = domen.replace('.','/');

                if (!lib.toObject()["url"].isNull())
                    baseUrl = lib.toObject()["url"].toString();

                // naming basename / domen / name / version / name-version [ -natives [-arch]] ".jar"

                QString suffix = domen + "/" + name + "/" + version + "/" + name + "-" + version;
                QString url = baseUrl + suffix;
                QString fname = baseDir + suffix;

                if (!lib.toObject()["natives"].isNull()) {

                    // Get all os and architecture natives
                    QStringList oslist;
                    oslist << "linux" << "windows" << "osx";

                    foreach (QString os, oslist) {

                        QApplication::processEvents();

                        // Check allow-disallow rules
                        QJsonArray rules = lib.toObject()["rules"].toArray();

                        bool allowLib = true;
                        if (!rules.isEmpty()) {

                            // Disallow libray if not in allow list
                            allowLib = false;

                            foreach (QJsonValue ruleValue, rules) {
                                QJsonObject rule = ruleValue.toObject();

                                // Process allow variants (all or specified)
                                if (rule["action"].toString() == "allow") {
                                    if (rule["os"].toObject().isEmpty()) {
                                        allowLib = true;
                                    } else if (rule["os"].toObject()["name"].toString() == os) {
                                        allowLib = true;
                                    }
                                }

                                // Make exclusions from allow-list
                                if (rule["action"].toString() == "disallow") {
                                    if (rule["os"].toObject()["name"].toString() == os) {
                                        allowLib = false;
                                    }
                                }
                            }
                        }

                        // Go to next lib entry, if this are disallowed
                        if (!allowLib) { continue; }

                        if (!lib.toObject()["natives"].toObject()[os].isNull()) {
                            QString natives = lib.toObject()["natives"].toObject()[os].toString();
                            if (natives.contains("${arch}")) {

                                QString n32 = natives;
                                QString n64 = natives;

                                // 32-bit version
                                n32.replace("${arch}", "32");
                                downloadFile(url + "-" + n32 + ".jar",
                                           fname + "-" + n32 + ".jar");
                                // 64-bit version
                                n64.replace("${arch}", "64");
                                downloadFile(url + "-" + n64 + ".jar",
                                           fname + "-" + n64 + ".jar");
                            } else {
                                downloadFile(url + "-" + natives + ".jar",
                                           fname + "-" + natives + ".jar");
                            }
                        }

                    }

                } else {
                    downloadFile(url + ".jar", fname + ".jar");
                }
            }

            // Fetch all assets
            ui->log->appendPlainText("Загрузка ресурсов... ");
            logger->append("FetchDialog", "Fetching resources... \n");

            QString assetsDir = settings->getAssetsDir();
            QString assetsVer = index["assets"].toString();

            if (assetsVer.isEmpty()) {

                ui->log->appendPlainText("Ошибка: не указан файл ресурсов (assets)");
                logger->append("FetchDialog", "Error assets id not found in version.jar \n");

            } else {

                Util::downloadFile("https://s3.amazonaws.com/Minecraft.Download/indexes/" + assetsVer + ".json",
                                                     assetsDir + "/indexes/" + assetsVer + ".json");
                QFile assetsFile(assetsDir + "/indexes/" + assetsVer + ".json");

                if (assetsFile.exists()) {
                    if (assetsFile.open(QIODevice::ReadOnly)) {

                        QJsonParseError error;
                        QJsonDocument assetsDoc = QJsonDocument::fromJson(assetsFile.readAll(), &error);
                        assetsFile.close();

                        if (error.error == QJsonParseError::NoError) {

                            QJsonObject assetsObjects = assetsDoc.object()["objects"].toObject();
                            QStringList keys = assetsObjects.keys();

                            foreach (QString key, keys) {

                                QApplication::processEvents();

                                QString hash = assetsObjects[key].toObject()["hash"].toString();
                                QString objectsUrl = "http://resources.download.minecraft.net/" + hash.mid(0, 2) + "/" + hash;
                                QString objectsDir = settings->getAssetsDir() + "/objects/" + hash.mid(0, 2) + "/" + hash;

                                downloadFile(objectsUrl, objectsDir);
                            }

                        } else {
                            ui->log->appendPlainText("Ошибка: не удалось разобрать JSON файл " + assetsFile.fileName());
                            logger->append("FetchDialog", "Error: can't parse JSON file " + assetsFile.fileName() + "\n");
                        }

                    } else {
                        ui->log->appendPlainText("Ошибка: не удалось открыть файл " + assetsFile.fileName());
                        logger->append("FetchDialog", "Error: can't open file " + assetsFile.fileName() + "\n");
                    }

                } else {
                    ui->log->appendPlainText("Ошибка: отсутствует " + assetsVer + ".json");
                    logger->append("FetchDialog", "Error: no file: " + assetsDir + "/indexes/" + assetsVer + ".json\n");
                }
            }

        } else {
            ui->log->appendPlainText("Ошибка: не удалось разобрать JSON файл " + indexName);
            logger->append("FetchDialog", "Error: can't parse JSON file " + indexName + "\n");
        }

    } else {
        ui->log->appendPlainText("Ошибка: не удалось открыть файл " + indexName);
        logger->append("FetchDialog", "Error: can't open file " + indexName + "\n");
    }

    // Explode error list
    foreach (QString errStr, errList) {
        ui->log->appendPlainText(errStr);
    }

    ui->clientCombo->setEnabled(true);
    ui->versionCombo->setEnabled(true);
    ui->fetchButton->setEnabled(true);

}

void FetchDialog::makeVersionList() {

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
        logger->append("FetchDialog", "WARN: Empty client list!\n");
    }

}
