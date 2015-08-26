#include "updatedialog.h"
#include "ui_updatedialog.h"

#include <QCryptographicHash>

#include "settings.h"
#include "logger.h"
#include "util.h"

UpdateDialog::UpdateDialog(QString displayMessage, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateDialog)
{
    ui->setupUi(this);

    dm = new DownloadManager(this);
    connect(dm, SIGNAL(progressChanged(int)), ui->progressBar, SLOT(setValue(int)));
    connect(dm, SIGNAL(beginDownloadFile(QString)), this, SLOT(downloadStarted(QString)));
    connect(dm, SIGNAL(error(QString)), this, SLOT(error(QString)));
    connect(dm, SIGNAL(finished()), this, SLOT(updateFinished()));

    settings = Settings::instance();
    logger = Logger::logger();
    logger->append("UpdateDialog", "Update dialog opened\n");

    ui->clientCombo->addItems(settings->getClientsNames());
    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
    connect(ui->clientCombo, SIGNAL(activated(int)), settings, SLOT(saveActiveClientId(int)));
    connect(ui->clientCombo, SIGNAL(activated(int)), this, SLOT(clientChanged()));

    state = canCheck;
    ui->log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->log->setPlainText(displayMessage);
    ui->updateButton->setText("Проверить");
    connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doCheck()));

    if (ui->clientCombo->count() == 0) {
        ui->updateButton->setEnabled(false);
        logger->append("UpdateDialog", "Error: empty client list!\n");
        ui->log->setPlainText("Ошибка! Не удалось получить список клиентов!");
    }
}

void UpdateDialog::clientChanged() {

    logger->append("UpdateDialog", "Selected client: " + settings->getClientStrId(settings->loadActiveClientId()) + "\n");
    ui->log->setPlainText("Для проверки наличия обновлений выберите нужный клиент и нажмите кнопку \"Проверить\"");

    switch (state) {

    case canUpdate:
        dm->reset();
        removeList.clear();
        ui->progressBar->setValue(0);

        disconnect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doUpdate()));

        ui->updateButton->setText("Проверить");
        connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doCheck()));
        state = canCheck;

    case canClose:
        dm->reset();
        removeList.clear();
        ui->progressBar->setValue(0);

        disconnect(ui->updateButton, SIGNAL(clicked()), this, SLOT(close()));

        ui->updateButton->setText("Проверить");
        connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doCheck()));
        state = canCheck;

    default:
    case canCheck:
        dm->reset();
        removeList.clear();
        ui->progressBar->setValue(0);
    }

}

void UpdateDialog::doCheck() {
    ui->clientCombo->setEnabled(false);
    ui->updateButton->setEnabled(false);

    ui->log->appendPlainText("");
    ui->log->appendPlainText("Проверка клиента " + settings->getClientStrId(settings->loadActiveClientId())
                             + ", версия " + settings->loadClientVersion());
    logger->append("UpdateDialog", "Checking client version: " + settings->loadClientVersion() + "\n");

    // Setup begin checking data

    bool needUpdate = false;
    clientVersion = settings->loadClientVersion();
    QString versionsDir = settings->getVersionsDir();

    // Check for latest version
    if (clientVersion == "latest") {

        ui->log->appendPlainText("Определение последней версии клиента...");
        logger->append("UpdateDialog", "Looking for latest version...\n");

        Reply versionReply = Util::makeGet(settings->getVersionsUrl());
        if (versionReply.isOK()) {

            QJsonParseError error;
            QJsonDocument jsonVersionReply = QJsonDocument::fromJson(versionReply.reply(), &error);

            if (error.error == QJsonParseError::NoError) {

                QJsonObject latest = jsonVersionReply.object()["latest"].toObject();

                if (latest["release"].isNull()) {
                    ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось определить последнюю версию клиента");
                    logger->append("UpdateDialog", "Error: empty latest client version\n");

                    ui->clientCombo->setEnabled(true);
                    ui->updateButton->setEnabled(true);
                    return;

                } else {
                    clientVersion = latest["release"].toString();
                }

            } else {
                ui->log->appendPlainText("Проверка остановлена. Ошибка разбора JSON!");
                logger->append("UpdateDialog", "Error: can't parse JSON\n");

                ui->clientCombo->setEnabled(true);
                ui->updateButton->setEnabled(true);
                return;
            }

        } else {
            ui->log->appendPlainText("Проверка остановлена. Ошибка: " + versionReply.getErrorString());
            logger->append("UpdateDialog", "Error: " + versionReply.getErrorString() + "\n");

            ui->clientCombo->setEnabled(true);
            ui->updateButton->setEnabled(true);
            return;
        }

    }

    // Check for version and data indexes
    ui->log->appendPlainText("\n # Проверка файлов игры:");
    logger->append("UpdateDialog", "Checking game files\n");

    QString versionFilePrefix = versionsDir + "/" + clientVersion + "/";
    QString versionUrlPrefix = settings->getVersionUrl(clientVersion);

    if (!downloadNow(versionUrlPrefix + clientVersion +".json",
                             versionFilePrefix + clientVersion +".json")) {

        ui->clientCombo->setEnabled(true);
        ui->updateButton->setEnabled(true);
        return;
    }

    if (!downloadNow(versionUrlPrefix + "data.json",
                             versionFilePrefix + "data.json")) {

        ui->clientCombo->setEnabled(true);
        ui->updateButton->setEnabled(true);
        return;
    }

    // Reading info from indexes
    QJsonDocument versionJson, dataJson;

    // Reading version index
    QFile* versionIndexfile = new QFile(versionFilePrefix + clientVersion +".json");
    if (!versionIndexfile->open(QIODevice::ReadOnly)) {

        ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось открыть " + clientVersion +".json");
        logger->append("UpdateDialog", "Error: can't open " + clientVersion +".json\n");

        ui->clientCombo->setEnabled(true);
        ui->updateButton->setEnabled(true);
        return;

    } else {

        QJsonParseError error;
        versionJson = QJsonDocument::fromJson(versionIndexfile->readAll(), &error);

        if (error.error != QJsonParseError::NoError) {

            ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось разобрать" + clientVersion +".json");
            logger->append("UpdateDialog", "Error: can't parse " + clientVersion +".json\n");

            ui->clientCombo->setEnabled(true);
            ui->updateButton->setEnabled(true);
            return;

        }
        versionIndexfile->close();
    }
    delete versionIndexfile;

    // Reading data index
    QFile* dataIndexfile = new QFile(versionFilePrefix + "data.json");
    if (!dataIndexfile->open(QIODevice::ReadOnly)) {

        ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось открыть data.json");
        logger->append("UpdateDialog", "Error: can't open data.json\n");

        ui->clientCombo->setEnabled(true);
        ui->updateButton->setEnabled(true);
        return;

    } else {

        QJsonParseError error;
        dataJson = QJsonDocument::fromJson(dataIndexfile->readAll(), &error);

        if (error.error != QJsonParseError::NoError) {

            ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось разобрать data.json");
            logger->append("UpdateDialog", "Error: can't parse data.json\n");

            ui->clientCombo->setEnabled(true);
            ui->updateButton->setEnabled(true);
            return;

        }
        dataIndexfile->close();
    }
    delete dataIndexfile;

    // Check game files
    QString url, fileName, displayName, checkSum;
    quint64 size;

    // Check main file
    url = versionUrlPrefix + clientVersion + ".jar";
    fileName = versionFilePrefix + clientVersion + ".jar";
    displayName = "файл " + clientVersion + ".jar";
    checkSum = dataJson.object()["main"].toObject()["hash"].toString();
    size = dataJson.object()["main"].toObject()["size"].toInt();

    if (addToQueryIfNeed(url, fileName, displayName, checkSum, size)) needUpdate = true;

    // Check libs
    ui->log->appendPlainText("\n # Проверка библиотек:");
    logger->append("UpdateDialog", "Checking libs\n");

    QString libFilePrefix = settings->getLibsDir() + "/";
    QString libUrlPrefix = settings->getLibsUrl();

    QJsonArray libraries = versionJson.object()["libraries"].toArray();
    foreach (QJsonValue libValue, libraries) {
        QJsonObject library = libValue.toObject();
        QStringList entry = library["name"].toString().split(':');

        // <package>:<name>:<version> to <package>/<name>/<version>/<name>-<version> and chahge <backage> format from a.b.c to a/b/c
        QString libSuffix = entry.at(0);                // package
        libSuffix.replace('.', '/');                    // package format
        libSuffix += "/" + entry.at(1)                  // + name
                + "/" + entry.at(2)                      // + version
                + "/" + entry.at(1) + "-" + entry.at(2); // + name-version

        // Check rules for disallow rules
        QJsonArray rules = library["rules"].toArray();
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
                    } else if (rule["os"].toObject()["name"].toString() == settings->getOsName()) {
                        allowLib = true;
                    }
                }

                // Make exclusions from allow-list
                if (rule["action"].toString() == "disallow") {
                    if (rule["os"].toObject()["name"].toString() == settings->getOsName()) {
                        allowLib = false;
                    }
                }
            }
        }
        // Go to next lib entry, if this are disallowed
        if (!allowLib) {
            logger->append("UpdateDialog", "Skipping lib: " + libSuffix + ".jar\n");
            continue;
        }

        // Check for natives entry: <package>/<name>-<version>-<native_string>
        QString nativesSuffix = library["natives"].toObject()[settings->getOsName()].toString();
        nativesSuffix.replace("${arch}", settings->getWordSize());
        if (!nativesSuffix.isEmpty()) {
            libSuffix += "-" + nativesSuffix + ".jar";
        } else {
            libSuffix += ".jar";
        }

        // Check each library file by exists and hash
        url = libUrlPrefix + libSuffix;
        fileName = libFilePrefix + libSuffix;
        displayName = "файл " + libSuffix.split('/').last();
        checkSum = dataJson.object()["libs"].toObject()[libSuffix].toObject()["hash"].toString();
        size = dataJson.object()["libs"].toObject()[libSuffix].toObject()["size"].toInt();

        if (addToQueryIfNeed(url, fileName, displayName, checkSum, size)) needUpdate = true;
        QApplication::processEvents(); // Update text in log
    }

    // Check assets
    ui->log->appendPlainText("\n # Проверка игровых ресурсов:");
    logger->append("UpdateDialog", "Checking assets\n");

    QJsonDocument assetsJson;
    QString assetsVersion = versionJson.object()["assets"].toString();
    QString assetsFilePrefix = settings->getAssetsDir() + "/objects/";
    QString assetsUrlPrefix = settings->getAssetsUrl() + "objects/";

    if (!downloadNow(settings->getAssetsUrl() + "indexes/" + assetsVersion + ".json",
                             settings->getAssetsDir() + "/indexes/" + assetsVersion + ".json")) {

        dm->reset();
        ui->clientCombo->setEnabled(true);
        ui->updateButton->setEnabled(true);
        return;
    }

    // Reading assets index
    QFile* assetsIndexfile = new QFile(settings->getAssetsDir() + "/indexes/" + assetsVersion + ".json");
    if (!assetsIndexfile->open(QIODevice::ReadOnly)) {

        ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось открыть " + assetsVersion +".json");
        logger->append("UpdateDialog", "Error: can't open assets index " + assetsVersion +".json\n");
        return;

    } else {

        QJsonParseError error;
        assetsJson = QJsonDocument::fromJson(assetsIndexfile->readAll(), &error);

        if (error.error != QJsonParseError::NoError) {

            ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось разобрать " + assetsVersion +".json");
            logger->append("UpdateDialog", "Error: can't parse assets index " + assetsVersion +".json\n");

            dm->reset();
            ui->clientCombo->setEnabled(true);
            ui->updateButton->setEnabled(true);
            return;

        }
        assetsIndexfile->close();
    }
    delete assetsIndexfile;

    // Check each asset by exists and hash
    QJsonObject assets = assetsJson.object()["objects"].toObject();

    foreach (QString key, assets.keys()) {
        QJsonObject asset = assets[key].toObject();

        // Check each asset file
        checkSum = asset["hash"].toString();
        size = asset["size"].toInt();
        url = assetsUrlPrefix + checkSum.mid(0, 2) + "/" + checkSum;
        fileName = assetsFilePrefix + checkSum.mid(0, 2) + "/" + checkSum;
        displayName = "ресурс " + key;

        if (addToQueryIfNeed(url, fileName, displayName, checkSum, size)) needUpdate = true;
        QApplication::processEvents(); // Update text in log
    }

    // Check additional files if defined
    if (!dataJson.object()["files"].toObject()["index"].isNull()) {

        ui->log->appendPlainText("\n # Проверка дополнительных модификаций:");
        logger->append("UpdateDialog", "Checking custom files\n");

        // Open installed files index
        QJsonDocument installedDataJson;

        if (QFile::exists(settings->getClientPrefix(clientVersion) + "/installed_data.json")) {

            ui->log->appendPlainText("Проверка наличия устаревших файлов...");
            logger->append("UpdateDialog", "Making deletion list...\n");

            QFile* installedDataFile = new QFile(settings->getClientPrefix(clientVersion) + "/installed_data.json");
            if (!installedDataFile->open(QIODevice::ReadOnly)) {

                ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось открыть installed_data.json");
                logger->append("UpdateDialog", "Error: can't open installed_files.json\n");

                dm->reset();
                ui->clientCombo->setEnabled(true);
                ui->updateButton->setEnabled(true);
                return;

            } else {

                QJsonParseError error;
                installedDataJson = QJsonDocument::fromJson(installedDataFile->readAll(), &error);

                if (error.error != QJsonParseError::NoError) {

                    ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось разобрать installed_data.json");
                    logger->append("UpdateDialog", "Error: can't parse installed_data.json\n");

                    dm->reset();
                    ui->clientCombo->setEnabled(true);
                    ui->updateButton->setEnabled(true);
                    return;

                }
                installedDataFile->close();
            }
            delete installedDataFile;

            // Check for difference between current and previous installations
            QStringList currentFileList = dataJson.object()["files"].toObject()["index"].toObject().keys();
            QStringList previousFileList = installedDataJson.object()["files"].toObject()["index"].toObject().keys();

            // Add file to deletion list if exist in previous installation and not exists in current
            foreach (QString installedEntry, previousFileList) {
                if (currentFileList.indexOf(installedEntry) == -1) {

                    removeList.append(installedEntry);
                    needUpdate = true;

                    ui->log->appendPlainText(" >> Необходимо удалить: " + installedEntry);
                    logger->append("UpdateDialog", "Marked to delete: " + installedEntry + "\n");
                }
            }
        }

        // Check custom files
        ui->log->appendPlainText("Проверка файлов модификаций...");
        logger->append("UpdateDialog", "Checking needed custom files...\n");

        // Make mutable files list (that checks only by existence)
        QStringList mutableList;
        foreach (QJsonValue value, dataJson.object()["files"].toObject()["mutables"].toArray()) {
            mutableList.append(value.toString());
        }

        QString filesFilePrefix = settings->getClientPrefix(clientVersion) + "/";
        QString filesUrlPrefix = settings->getVersionUrl(clientVersion) + "files/";

        foreach (QString key, dataJson.object()["files"].toObject()["index"].toObject().keys()) {

            QJsonObject customFile
                    = dataJson.object()["files"].toObject()["index"].toObject()[key].toObject();

            // Check each custom file
            checkSum = customFile["hash"].toString();
            size = customFile["size"].toInt();
            url = filesUrlPrefix + key;
            fileName = filesFilePrefix + key;
            displayName = "файл " + key;

            if (mutableList.contains(key)) checkSum = "mutable"; // Download only if not exists

            if (addToQueryIfNeed(url, fileName, displayName, checkSum, size)) needUpdate = true;
            QApplication::processEvents(); // Update text in log
        }
    }

    if (needUpdate) {

        disconnect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doCheck()));
        ui->updateButton->setText("Обновить");
        connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doUpdate()));
        state = canUpdate;

        ui->log->appendPlainText("\nТребуется обновление!");

        if (!removeList.isEmpty()) {

            ui->log->appendPlainText("Необходимо удалить: " + removeList.join(", "));
            logger->append("UpdateDialog", "Check result: remove list: " + removeList.join(", ") + "\n");
        }

        if (dm->getDownloadsSize() != 0) {

            ui->log->appendPlainText("Необходимо загрузить "
                                     + QString::number((float(dm->getDownloadsSize()) / 1024 / 1024), 'f', 2)
                                     + " МиБ");
            logger->append("UpdateDialog", "Check result: need to download "
                           + QString::number((float(dm->getDownloadsSize()) / 1024 / 1024), 'f', 2)
                           + " MiB\n");
        }

    } else {
        ui->log->appendPlainText("\nОбновление не требуется!");
        logger->append("UpdateDialog", "Check result: no need updates\n");

        disconnect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doCheck()));
        ui->updateButton->setText("Закрыть");
        connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(close()));
        state = canClose;
    }

    ui->clientCombo->setEnabled(true);
    ui->updateButton->setEnabled(true);
}

void UpdateDialog::doUpdate() {
    ui->clientCombo->setEnabled(false);
    ui->updateButton->setEnabled(false);

    if (!removeList.isEmpty()) {

        ui->log->appendPlainText("\n # Удаление устаревших модификаций:");
        logger->append("UpdateDialog", "Removing files...\n");

        foreach (QString entry, removeList) {
            ui->log->appendPlainText("Удаление: " + entry);
            logger->append("UpdateDialog", "Remove " + entry +"\n");

            QFile::remove(settings->getClientPrefix(clientVersion) + "/" + entry);
            ui->progressBar->setValue(int((float(removeList.indexOf(entry) + 1) / removeList.size()) * 100));
        }

    }

    // Replace custom files index
    QFile::remove(settings->getClientPrefix(clientVersion) + "/installed_data.json");

    QDir(settings->getClientPrefix(clientVersion) + "/").mkpath(settings->getClientPrefix(clientVersion) + "/");
    QFile::copy(settings->getVersionsDir() + "/" + clientVersion + "/data.json",
                settings->getClientPrefix(clientVersion) + "/installed_data.json");

    if (dm->getDownloadsSize() != 0) {

        ui->log->appendPlainText("\n # Загрузка обновлений:");
        logger->append("UpdateDialog", "Downloading started...\n");
        dm->startDownloads();

    } else {
        updateFinished();
    }

}

void UpdateDialog::downloadStarted(QString displayName) {
    ui->log->appendPlainText("Загружается " + displayName);
}

void UpdateDialog::error(QString errorString) {
    ui->log->appendPlainText(" [!] Ошибка: " + errorString);
}

void UpdateDialog::updateFinished() {

    ui->log->appendPlainText("\nОбновление выполнено!");
    logger->append("UpdateDialog", "Update completed\n");

    disconnect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doUpdate()));
    ui->updateButton->setText("Закрыть");
    connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(close()));
    state = canUpdate;

    ui->clientCombo->setEnabled(true);
    ui->updateButton->setEnabled(true);
}

bool UpdateDialog::downloadNow(QString url, QString fileName) {

    ui->log->appendPlainText("Загрузка: " + fileName.split("/").last());
    logger->append("UpdateDialog", "Downloading "  + fileName.split("/").last() + "\n");

    Reply reply = Util::makeGet(url);
    if (!reply.isOK()) {
        ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось загрузить файл");
        logger->append("UpdateDialog", "Error: " + reply.getErrorString() + "\n");
        return false;
    } else {
        QFile* file = new QFile(fileName);

        QDir fdir = QFileInfo(fileName).absoluteDir();
        fdir.mkpath(fdir.absolutePath());

        if (file->open(QIODevice::WriteOnly)) {
            file->write(reply.reply());
            file->close();
            delete file;
            return true;
        } else {
            ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось сохранить файл");
            logger->append("UpdateDialog", "Error: can't save file\n");
            delete file;
            return false;
        }
    }

    return true;
}

bool UpdateDialog::addToQueryIfNeed(QString url, QString fileName, QString displayName, QString checkSum, quint64 size) {

    ui->log->appendPlainText("Проверка: " + displayName);
    logger->append("UpdateDialog", "Checking " + fileName + "\n");

    if (!QFile::exists(fileName)) {

        logger->append("UpdateDialog", "Checking: file does not exist\n");
        dm->addEntry(url, fileName, displayName, size);
        ui->log->appendPlainText(" >> Необходимо загрузить " + displayName + " ("
                                 + QString::number((float(size) / 1024 / 1024), 'f', 2) + " МиБ)" );
        return true;

    } else if (checkSum != "mutable") {

        QString fileHash;
        QFile* file = new QFile(fileName);
        if (!file->open(QIODevice::ReadOnly)) {
            ui->log->appendPlainText("Ошибка: не удалось открыть " + displayName);
            logger->append("UpdateDialog", "Error: can't open file " + fileName.split("/").last() + "\n");
            delete file;
            return false;
        } else {
            QByteArray data = file->readAll();
            fileHash = QString(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex());
            file->close();
            delete file;

            if (fileHash != checkSum) {

                logger->append("UpdateDialog", "Checking: bad checksum\n");
                dm->addEntry(url, fileName, displayName, size);
                ui->log->appendPlainText(" >> Необходимо загрузить " + displayName + " ("
                                         + QString::number((float(size) / 1024 / 1024), 'f', 2) + " МиБ)" );
                return true;
            }
        }
    }

    return false;
}

UpdateDialog::~UpdateDialog() {

    logger->append("UpdateDialog", "Update dialog closed\n");
    delete ui;
}
