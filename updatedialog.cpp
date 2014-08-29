#include "updatedialog.h"
#include "ui_updatedialog.h"

#include <QCryptographicHash>

#include "settings.h"
#include "logger.h"
#include "util.h"

UpdateDialog::UpdateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateDialog)
{
    ui->setupUi(this);

    dm = new DownloadManager(this);
    connect(dm, SIGNAL(progressChanged(int)), ui->progressBar, SLOT(setValue(int)));
    connect(dm, SIGNAL(beginDownloadFile(QString)), this, SLOT(downloadStarted(QString)));
    connect(dm, SIGNAL(error(QString)), this, SLOT(error(QString)));
    connect(dm, SIGNAL(finished()), this, SLOT(downloadsFinished()));

    settings = Settings::instance();
    logger = Logger::logger();
    logger->append("UpdateDialog", "Update dialog opened\n");

    ui->clientCombo->addItems(settings->getClientsNames());
    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
    connect(ui->clientCombo, SIGNAL(activated(int)), settings, SLOT(saveActiveClientId(int)));
    connect(ui->clientCombo, SIGNAL(activated(int)), this, SLOT(clientChanged()));

    if (ui->clientCombo->count() == 0) {
        ui->updateButton->setEnabled(false);
        logger->append("UpdateDialog", "Error: empty client list!\n");
        ui->log->setPlainText("Ошибка! Не удалось получить список клиентов!");
    } else {
        // Auto switch to "check state"
        updateState = true;
        emit ui->clientCombo->activated(ui->clientCombo->currentIndex());
    }


}

void UpdateDialog::clientChanged() {

    logger->append("UpdateDialog", "Selected client: " + settings->getClientStrId(settings->loadActiveClientId()) + "\n");
    ui->log->setPlainText("Для проверки наличия обновлений выберите нужный клиет и нажмите кнопку \"Проверить\"");

    if (updateState) {

        dm->reset();
        ui->progressBar->setValue(0);
        updateState = false;
        disconnect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doUpdate()));

        ui->updateButton->setText("Проверить");
        ui->updateButton->setEnabled(true);
        connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doCheck()));
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
    QString clientVersion = settings->loadClientVersion();
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
                    ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось определить оследнюю версию клиента");
                    logger->append("UpdateDialog", "Error: empty latest client version\n");
                    return;
                } else {
                    clientVersion = latest["release"].toString();
                }

            } else {
                ui->log->appendPlainText("Проверка остановлена. Ошибка разбора JSON!");
                logger->append("UpdateDialog", "Error: can't parse JSON\n");
                return;
            }

        } else {
            ui->log->appendPlainText("Проверка остановлена. Ошибка: " + versionReply.getErrorString());
            logger->append("UpdateDialog", "Error: " + versionReply.getErrorString() + "\n");
            return;
        }

    }

    // Check for version-files
    ui->log->appendPlainText("\n # Проверка файлов игры:");
    logger->append("UpdateDialog", "Checking game files\n");

    QString versionFilePrefix = versionsDir + "/" + clientVersion + "/";
    QString versionUrlPrefix = settings->getVersionUrl(clientVersion);

    if (!downloadIfNotExists(versionUrlPrefix + clientVersion +".json",
                             versionFilePrefix + clientVersion +".json")) {
        return;
    }

    if (!downloadIfNotExists(versionUrlPrefix + "libs.json",
                             versionFilePrefix + "libs.json")) {
        return;
    }

    // Reading info from JSON-files files
    QJsonDocument versionJson, libsJson;

    QFile* versionIndexfile = new QFile(versionFilePrefix + clientVersion +".json");
    if (!versionIndexfile->open(QIODevice::ReadOnly)) {

        ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось открыть " + clientVersion +".json");
        logger->append("UpdateDialog", "Error: can't open " + clientVersion +".json\n");
        return;

    } else {

        QJsonParseError error;
        versionJson = QJsonDocument::fromJson(versionIndexfile->readAll(), &error);

        if (error.error != QJsonParseError::NoError) {

            ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось разобрать" + clientVersion +".json");
            logger->append("UpdateDialog", "Error: can't parse " + clientVersion +".json\n");
            return;

        }
        versionIndexfile->close();
    }
    delete versionIndexfile;

    QFile* libsIndexfile = new QFile(versionFilePrefix + "libs.json");
    if (!libsIndexfile->open(QIODevice::ReadOnly)) {

        ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось открыть libs.json");
        logger->append("UpdateDialog", "Error: can't open libs.json\n");
        return;

    } else {

        QJsonParseError error;
        libsJson = QJsonDocument::fromJson(libsIndexfile->readAll(), &error);

        if (error.error != QJsonParseError::NoError) {

            ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось разобрать libs.json");
            logger->append("UpdateDialog", "Error: can't parse libs.json\n");
            return;

        }
        libsIndexfile->close();
    }
    delete libsIndexfile;

    // Check game files
    QString url, fileName, displayName, checkSumm;
    quint64 size;

    // Check main file
    url = versionUrlPrefix + clientVersion + ".jar";
    fileName = versionFilePrefix + clientVersion + ".jar";
    displayName = "файл " + clientVersion + ".jar";
    checkSumm = versionJson.object()["jarHash"].toString();
    size = versionJson.object()["jarSize"].toInt();

    if (addToQueryIfNeed(url, fileName, displayName, checkSumm, size)) needUpdate = true;

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
            foreach (QJsonValue ruleValue, rules) {
                QJsonObject rule = ruleValue.toObject();

                // Disallow libray if not in allow list
                allowLib = false;

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
        if (!allowLib) continue;

        // Check for natives entry: <package>/<name>-<version>-<native_string>
        QString nativesSuffix = library["natives"].toObject()[settings->getOsName()].toString();
        nativesSuffix.replace("${arch}", settings->getWordSize());
        if (!nativesSuffix.isEmpty()) {
            libSuffix += "-" + nativesSuffix + ".jar";
        } else {
            libSuffix += ".jar";
        }

        // Check each library file
        url = libUrlPrefix + libSuffix;
        fileName = libFilePrefix + libSuffix;
        displayName = "файл " + libSuffix.split('/').last();
        checkSumm = libsJson.object()["objects"].toObject()[libSuffix].toObject()["hash"].toString();
        size = libsJson.object()["objects"].toObject()[libSuffix].toObject()["size"].toInt();

        if (addToQueryIfNeed(url, fileName, displayName, checkSumm, size)) needUpdate = true;
    }

    // Check assets
    ui->log->appendPlainText("\n # Проверка игровых ресурсов:");
    logger->append("UpdateDialog", "Checking assets\n");

    QJsonDocument assetsJson;
    QString assetsVersion = versionJson.object()["assets"].toString();
    QString assetsFilePrefix = settings->getAssetsDir() + "/objects/";
    QString assetsUrlPrefix = settings->getAssetsUrl() + "objects/";

    if (!downloadIfNotExists(settings->getAssetsUrl() + "indexes/" + assetsVersion + ".json",
                             settings->getAssetsDir() + "/indexes/" + assetsVersion + ".json")) {
        return;
    }

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
            return;

        }
        assetsIndexfile->close();
    }
    delete assetsIndexfile;

    QJsonObject assets = assetsJson.object()["objects"].toObject();
    foreach (QString key, assets.keys()) {
        QJsonObject asset = assets[key].toObject();

        // Check each asset file
        checkSumm = asset["hash"].toString();
        size = asset["size"].toInt();
        url = assetsUrlPrefix + checkSumm.mid(0, 2) + "/" + checkSumm;
        fileName = assetsFilePrefix + checkSumm.mid(0, 2) + "/" + checkSumm;
        displayName = "ресурс " + key;

        if (addToQueryIfNeed(url, fileName, displayName, checkSumm, size)) needUpdate = true;
    }

    // Check custom files
    if (versionJson.object()["customFiles"].toBool()) {

        ui->log->appendPlainText("\n # Проверка дополнительных модификаций:");
        logger->append("UpdateDialog", "Checking ustom files\n");

        // Download files index
        if (!downloadIfNotExists(versionUrlPrefix + "files.json",
                                 versionFilePrefix + "files.json")) {
            return;
        }

        // Open custom files index
        QJsonDocument filesJson;

        QFile* filesIndexfile = new QFile(versionFilePrefix + "files.json");
        if (!filesIndexfile->open(QIODevice::ReadOnly)) {

            ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось открыть files.json");
            logger->append("UpdateDialog", "Error: can't open files.json\n");
            return;

        } else {

            QJsonParseError error;
            filesJson = QJsonDocument::fromJson(filesIndexfile->readAll(), &error);

            if (error.error != QJsonParseError::NoError) {

                ui->log->appendPlainText("Проверка остановлена. Ошибка: не удалось разобрать files.json");
                logger->append("UpdateDialog", "Error: can't parse files.json\n");
                return;

            }
            filesIndexfile->close();
        }
        delete filesIndexfile;

        // FIXME: need to apply delete rules here!

        // Make mutable list
        QStringList mutableList;
        foreach (QJsonValue value, filesJson.object()["mutable"].toArray()) {
            mutableList.append(value.toString());
        }

        QString filesFilePrefix = settings->getClientDir() + "/";
        QString filesUrlPrefix = settings->getVersionUrl(clientVersion) + "files/";

        foreach (QString key, filesJson.object()["objects"].toObject().keys()) {

            QJsonObject customFile
                    = filesJson.object()["objects"].toObject()[key].toObject();

            // Check each asset file
            checkSumm = customFile["hash"].toString();
            size = customFile["size"].toInt();
            url = filesUrlPrefix + key;
            fileName = filesFilePrefix + key;
            displayName = "файл " + key;

            if (mutableList.contains(key)) checkSumm = "mutable"; // Download only if not exists

            if (addToQueryIfNeed(url, fileName, displayName, checkSumm, size)) needUpdate = true;
        }
    }

    if (needUpdate) {

        updateState = true;
        disconnect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doCheck()));

        ui->updateButton->setText("Обновить");
        connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doUpdate()));

        ui->log->appendPlainText("\nТребуется обновление! Необходимо загрузить "
                                 + QString::number((float(dm->getDownloadsSize()) / 1024 / 1024), 'f', 2)
                                 + " МиБ");
        logger->append("UpdateDialog", "Check result: need to download "
                       + QString::number((float(dm->getDownloadsSize()) / 1024 / 1024), 'f', 2)
                       + " MiB\n");

    } else {
        ui->log->appendPlainText("\nОбновление не требуется!");
        logger->append("UpdateDialog", "Check result: no need updates\n");
    }

    ui->clientCombo->setEnabled(true);
    ui->updateButton->setEnabled(true);
}

void UpdateDialog::doUpdate() {
    ui->clientCombo->setEnabled(false);
    ui->updateButton->setEnabled(false);

    ui->log->appendPlainText("\n\n # Обновление клиента:");
    logger->append("UpdateDialog", "Updates started...\n");
    dm->startDownloads();
}

void UpdateDialog::downloadStarted(QString displayName) {
    ui->log->appendPlainText("Загружается " + displayName);
}

void UpdateDialog::error(QString errorString) {
    ui->log->appendPlainText(" [!] Ошибка при загрузке: " + errorString);
}

void UpdateDialog::downloadsFinished() {
    ui->clientCombo->setEnabled(true);

    ui->log->appendPlainText("\nОбновление выполнено! Удалось загрузить "
                             + QString::number(ui->progressBar->value()) + "% данных.");
    logger->append("UpdateDialog", "Update finished with "
                   + QString::number(ui->progressBar->value()) + "% sucess.");
}

bool UpdateDialog::downloadIfNotExists(QString url, QString fileName) {

    if (!QFile::exists(fileName)) {
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
    }

    return true;
}

bool UpdateDialog::addToQueryIfNeed(QString url, QString fileName, QString displayName, QString checkSumm, quint64 size) {

    ui->log->appendPlainText("Проверка: " + displayName);
    logger->append("UpdateDialog", "Checking " + fileName + "\n");

    if (!QFile::exists(fileName)) {

        logger->append("UpdateDialog", "Checking: file not exists\n");
        dm->addEntry(url, fileName, displayName, size);
        ui->log->appendPlainText(" >> Необходимо загрузить " + displayName + " ("
                                 + QString::number((float(size) / 1024 / 1024), 'f', 2) + " МиБ)" );
        return true;

    } else if (checkSumm != "mutable") {

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

            if (fileHash != checkSumm) {

                logger->append("UpdateDialog", "Checking: bad checksumm\n");
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
