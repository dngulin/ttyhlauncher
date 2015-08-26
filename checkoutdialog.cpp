#include "checkoutdialog.h"
#include "ui_checkoutdialog.h"

#include "util.h"
#include <QCryptographicHash>

CheckoutDialog::CheckoutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CheckoutDialog)
{
    ui->setupUi(this);
    settings = Settings::instance();
    logger = Logger::logger();

    ui->mutableList->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    ui->clientCombo->addItems(settings->getClientsNames());

    connect(ui->clientCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(makeVersionList()));
    makeVersionList();

    connect(ui->checkoutButton, SIGNAL(clicked()), this, SLOT(makeCheckout()));
}

CheckoutDialog::~CheckoutDialog() {
    delete ui;
}

QPair<QString, int> CheckoutDialog::getHashAndSize(QString fname) {

    QFile file(fname);
    QString hash;
    int size;

    ui->log->appendPlainText("Обработка файла: " + fname);
    logger->append("CheckoutDialog", "Checkout: " + fname + "\n");

    if (!file.open(QIODevice::ReadOnly)) {

        hash = "cant_open_file";
        size = 0;
        errList << QString("Не удалось открыть: " + fname);

    } else {

        QString jarHash = QString(QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha1).toHex());
        file.close();

        hash = jarHash;
        size = file.size();
    }

    QPair<QString, int> rvalue;
    rvalue.first = hash;
    rvalue.second = size;

    return rvalue;
}

void CheckoutDialog::makeCheckout() {

    ui->log->clear();
    errList.clear();
    errList << "-----" << "Список ошибок при расчёте контрольных сумм";

    ui->log->appendPlainText("Начинаю расчёт контрольных сумм...");
    logger->append("CheckoutDialog", "Begin checkout...\n");

    if ( (ui->clientCombo->count() < 1) || (ui->versionCombo->count() < 1) ) {
        ui->log->appendPlainText("ОШИБКА: Не указан клиент или версия!");
        logger->append("CheckoutDialog", "Error: Client or version not specified!\n");
        return;
    }

    ui->clientCombo->setEnabled(false);
    ui->versionCombo->setEnabled(false);
    ui->checkoutButton->setEnabled(false);

    // Prepare strings
    QString version = ui->versionCombo->currentText();
    QString dataDir = settings->getBaseDir() + "/client_"
                   + settings->getClientStrId(ui->clientCombo->currentIndex()) + "/"
                   + "versions/" + version + "/";

    // Prepare JSON objects
    QJsonObject dataObject;
    QJsonObject main, libs, files;

    // Setup main section
    ui->log->appendPlainText("Секция: main");
    logger->append("CheckoutDialog", "Section: main\n");

    QPair<QString, int> hashAndSize = getHashAndSize(dataDir + version + ".jar");
    main["hash"] = hashAndSize.first;
    main["size"] = hashAndSize.second;

    // Setup libs section
    ui->log->appendPlainText("Секция: libs");
    logger->append("CheckoutDialog", "Section: libs\n");

    // Open index file (for libs enumerate)
    QFile indexFile(dataDir + version + ".json");
    if (!indexFile.open(QIODevice::ReadOnly)) {

        ui->log->appendPlainText("ОШИБКА: Не удалось открыть индекс!");
        logger->append("CheckoutDialog", "Error: Cant open index.json\n");
        return;

    } else {

        QJsonDocument indexDoc;
        QJsonArray libsIndex;
        QJsonParseError error;

        indexDoc = QJsonDocument::fromJson(indexFile.readAll(), &error);
        indexFile.close();

        if (error.error != QJsonParseError::NoError) {

            ui->log->appendPlainText("ОШИБКА: Не удалось разобрать индекс!");
            logger->append("CheckoutDialog", "Error: Cant parse index.json\n");
            return;

        } else {

            libsIndex = indexDoc.object()["libraries"].toArray();

            foreach (QJsonValue lib, libsIndex) {

                QString baseDir = settings->getLibsDir() + "/";
                QString codedName = lib.toObject()["name"].toString();

                QString domen, name, version;
                domen =   codedName.split(':')[0];
                name =    codedName.split(':')[1];
                version = codedName.split(':')[2];

                domen = domen.replace('.','/');

                QString suffix = domen + "/" + name + "/" + version + "/" + name + "-" + version;
                QString fname = baseDir + suffix;

                if (!lib.toObject()["natives"].isNull()) {

                    // Get all os and architecture natives
                    QStringList oslist;
                    oslist << "linux" << "windows" << "osx";

                    foreach (QString os, oslist) {

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
                                QPair<QString, int> hashAndSize;
                                QJsonObject libObj;

                                // 32-bit version
                                n32.replace("${arch}", "32");
                                hashAndSize = getHashAndSize(fname + "-" + n32 + ".jar");
                                libObj["hash"] = hashAndSize.first;
                                libObj["size"] = hashAndSize.second;
                                libs[suffix + "-" + n32 + ".jar"] = libObj;

                                // 64-bit version
                                n64.replace("${arch}", "64");
                                hashAndSize = getHashAndSize(fname + "-" + n64 + ".jar");
                                libObj["hash"] = hashAndSize.first;
                                libObj["size"] = hashAndSize.second;
                                libs[suffix + "-" + n64 + ".jar"] = libObj;

                            } else {

                                QPair<QString, int> hashAndSize = getHashAndSize(fname + "-" + natives + ".jar");

                                QJsonObject libObj;
                                libObj["hash"] = hashAndSize.first;
                                libObj["size"] = hashAndSize.second;
                                libs[suffix + "-" + natives + ".jar"] = libObj;

                            }
                        }
                    }

                    QApplication::processEvents();

                } else {

                    // Do checksumm
                    QPair<QString, int> hashAndSize = getHashAndSize(fname + ".jar");

                    QJsonObject libObj;
                    libObj["hash"] = hashAndSize.first;
                    libObj["size"] = hashAndSize.second;
                    libs[suffix + ".jar"] = libObj;

                }

                QApplication::processEvents();

            }
        }
    }


    // Setup files section
    ui->log->appendPlainText("Секция: files");
    logger->append("CheckoutDialog", "Section: files\n");

    QJsonArray mutables;
    QJsonObject index;

    // Make mutables list
    QStringList mutableNames = ui->mutableList->toPlainText().split('\n');
    foreach (QString mutableName, mutableNames) {
        mutables.append(mutableName);
    }

    // Make index
    QDir filesDir(dataDir + "files/");
    if (!filesDir.exists()) {

        ui->log->appendPlainText("Ошибка: директория не сущетвувет: " + dataDir + "files/");
        logger->append("CheckoutDialog", "Error: files directory not exists\n");
        errList << QString("Директория модификаций не существует: " + dataDir + "files/");

    } else {

        QStringList fileList;
        Util::recursiveFlist(&fileList, "", dataDir + "files/");

        foreach (QString file, fileList) {

            QJsonObject fileObj;
            QPair<QString, int> hashAndSize = getHashAndSize(dataDir + "files/" + file);
            fileObj["hash"] = hashAndSize.first;
            fileObj["size"] = hashAndSize.second;

            index[file] = fileObj;
        }

    }

    files["mutables"] = mutables;
    files["index"] = index;

    // Write checkout file
    dataObject["main"] = main;
    dataObject["libs"] = libs;
    dataObject["files"] = files;

    QJsonDocument dataJson;
    dataJson.setObject(dataObject);

    QFile dataFile(dataDir + "data.json");
    if (!dataFile.open(QIODevice::WriteOnly)) {
        errList << QString("Не удалось сохранить data-файл!");
    } else {
        dataFile.write(dataJson.toJson());
        dataFile.close();
    }

    // Setup client-side prefix (for build testing)
    ui->log->appendPlainText("Установка префикса...");
    logger->append("CheckoutDialog", "Setup prefix...\n");

    QString prefixDir = settings->getBaseDir() + "/client_"
                   + settings->getClientStrId(ui->clientCombo->currentIndex()) + "/"
                   + "prefixes/" + version + "/";

    QFile installedDataFile(prefixDir + "installed_data.json");
    if (!installedDataFile.open(QIODevice::ReadOnly)) {

        ui->log->appendPlainText("Установка префикса: не удалось открыть installed_data.json");
        logger->append("CheckoutDialog", "Setup prefix: cant open installed_data.json\n");


    } else {

        QJsonDocument installedDoc;
        QJsonParseError error;

        installedDoc = QJsonDocument::fromJson(installedDataFile.readAll(), &error);
        installedDataFile.close();

        if (error.error != QJsonParseError::NoError) {

            ui->log->appendPlainText("Ошибка: невозможно разобрать installed_data.json! Префикс повреждён!");
            logger->append("CheckoutDialog", "Error: can't parse installed_data.json! Broken prefix!\n");
            errList << QString("Ошибка разбора installed_data.json! Префикс повреждён!");


        } else {

            // Remove old files, that not exists in new data.json
            QStringList oldFileList = installedDoc.object()["files"].toObject()["index"].toObject().keys();
            QStringList newFileList = index.keys();

            foreach (QString file, oldFileList) {
                if (newFileList.indexOf(file) == -1) {

                    ui->log->appendPlainText("Удаление файла: " + file);
                    logger->append("CheckoutDialog", "Removing file: " + prefixDir + file + "\n");
                    QFile::remove(prefixDir + file);

                }
            }
        }
    }

    ui->log->appendPlainText("Копирование новых файлов...");
    logger->append("CheckoutDialog", "Begin prefix installation...");

    // Copy data.json to installed_data.json
    QFileInfo fileInf(prefixDir + "installed_data.json");
    if (fileInf.exists()) {
        QFile::remove(prefixDir + "installed_data.json");
    } else {
        QDir fdir = fileInf.absoluteDir();
        fdir.mkpath(fdir.absolutePath());
    }
    QFile::copy(dataDir + "data.json", prefixDir + "installed_data.json");

    // Copy files
    foreach (QString fname, index.keys()) {
        ui->log->appendPlainText("Копирование: " + fname);
        logger->append("CheckoutDialog", "Copy: " + fname);

        QFileInfo fileInf(prefixDir + fname);
        if (fileInf.exists()) {
            QFile::remove(prefixDir + fname);
        } else {
            QDir fdir = fileInf.absoluteDir();
            fdir.mkpath(fdir.absolutePath());
        }

        QFile::copy(dataDir + "files/" + fname, prefixDir + fname);
    }

    ui->log->appendPlainText("Вычисление контрольных сумм завершено!");
    logger->append("CheckoutDialog", "Checkout completed!");

    // Explode error list
    foreach (QString errStr, errList) {
        ui->log->appendPlainText(errStr);
    }

    ui->clientCombo->setEnabled(true);
    ui->versionCombo->setEnabled(true);
    ui->checkoutButton->setEnabled(true);

}

void CheckoutDialog::makeVersionList() {
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
        logger->append("CheckoutDialog", "WARN: Empty client list!\n");
    }
}
