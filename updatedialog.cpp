#include "updatedialog.h"
#include "ui_updatedialog.h"

#include "settings.h"

#include <QNetworkReply>

UpdateDialog::UpdateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateDialog)
{
    ui->setupUi(this);

    downloadSize = 0;
    assets = "";

    downloadNam = new QNetworkAccessManager(this);

    settings = Settings::instance();
    ui->clientCombo->addItems(settings->getClientsNames());
    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
    connect(ui->clientCombo, SIGNAL(activated(int)), settings, SLOT(saveActiveClientId(int)));
    connect(ui->clientCombo, SIGNAL(activated(int)), this, SLOT(clientChanged()));

    this->show();
    emit ui->clientCombo->activated(ui->clientCombo->currentIndex());

    connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(doUpdate()));
}

void UpdateDialog::clientChanged(){
    downloadSize = 0;
    downloadNames.clear();
    downloadUrls.clear();

    ui->updateButton->setEnabled(false);
    ui->clientCombo->setEnabled(false);
    ui->log->clear();

    Settings* settings = Settings::instance();
    QString clientStrId = settings->getClientStrId(settings->loadActiveClientId());
    ui->log->appendPlainText("Проверка наличия обновлений для клиента \""
                             + clientStrId + "\", версия \""
                             + settings->loadClientVersion() + "\"");

    // Find latest version and check connection to update-server
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl("https://s3.amazonaws.com/Minecraft.Download/versions/versions.json"));
    QNetworkReply* reply = manager->get(request);

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (isLatestVersionKnown(reply)) {

        bool doNext = true;
        if (doNext) doNext = checkVersionFiles();
        if (doNext) doNext = checkLibs();
        if (doNext) doNext = checkAssets();
        if (doNext) doNext = checkMods();
        checksResult(doNext);

        ui->clientCombo->setEnabled(true);

    } else {
        ui->log->appendPlainText("Не удалось связаться с сервером обновлений!");
        ui->clientCombo->setEnabled(true);
        ui->updateButton->setEnabled(true);
    }

    delete manager;
}

bool UpdateDialog::checkVersionFiles() {
    ui->log->appendPlainText("\n1. Проверка состояния базовых файлов...");
    QString filePrefix = settings->getClientDir() + "/versions";

    // Mke filename and URL prefixes
    QString version = settings->loadClientVersion();
    if (version == "latest") version = latestver;
    filePrefix += "/" + version + "/" + version;

    QString verUrl = "http://s3.amazonaws.com/Minecraft.Download/versions/" + version + "/" + version;

    // Check for file exists
    QFile* verIndex = new QFile(filePrefix + ".json");
    if (!verIndex->exists()) {
        if (!downloadNow(QUrl(verUrl + ".json"), verIndex->fileName())) return false;
    }
    delete verIndex;
    ui->progressBar->setValue(1);

    QFile* gameJar = new QFile(filePrefix + ".jar");
    if (!gameJar->exists()) {
        addTarget(QUrl(verUrl + ".jar"), gameJar->fileName());
    }
    delete gameJar;
    ui->progressBar->setValue(2);

    return true;
}

bool UpdateDialog::checkLibs() {
    ui->log->appendPlainText("\n2. Проверка состояния библиотек...");

    QString filePrefix = settings->getClientDir() + "/versions";

    // Mke filename and URL prefixes
    QString version = settings->loadClientVersion();
    if (version == "latest") version = latestver;
    filePrefix += "/" + version + "/" + version;

    QFile* cfgFile = new QFile(filePrefix + ".json");
    if (!cfgFile->open(QIODevice::ReadOnly)) {
        ui->log->appendPlainText("Не удалось открыть конфигурационный файл :(");
        return false;
    }

    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(cfgFile->readAll(), &error);
    cfgFile->close();
    delete cfgFile;

    if (!error.error == QJsonParseError::NoError) {
        ui->log->appendPlainText("Ошибка разбора конфигурационного файла :(");
        return false;
    }

    filePrefix = settings->getClientDir() + "/libraries";
    QJsonArray libraries = json.object()["libraries"].toArray();
    QJsonObject library;
    for (QJsonArray::iterator libit = libraries.begin(), end = libraries.end(); libit != end; ++libit) {
        ui->progressBar->setValue( ((float)libit.i / (end.i - 1)) * 58 + 2 );
        library = (*libit).toObject();
        QStringList entry = library["name"].toString().split(':');

        // <package>:<name>:<version> to <package>/<name>/<version>/<name>-<version> and chahge <backage> format from a.b.c to a/b/c
        QString fileSuffix = entry.at(0);                    // package
        fileSuffix.replace('.', '/');                        // package format
        fileSuffix += "/" + entry.at(1)                      // + name
                    + "/" + entry.at(2)                      // + version
                    + "/" + entry.at(1) + "-" + entry.at(2); // + name-verion

        // Check rules for disallow rules
        QJsonArray rules = library["rules"].toArray();
        QJsonObject rule;
        bool allow = true;
        if (!rules.isEmpty()) {
            for (QJsonArray::iterator ruleit = rules.begin(), end = rules.end(); ruleit != end; ++ruleit) {
                rule = (*ruleit).toObject();

                // Disallow libray if not in allow list
                allow = false;

                // Process allow variants (all or specified)
                if (rule["action"].toString() == "allow") {
                    if (rule["os"].toObject().isEmpty()) {
                        allow = true;
                    } else if (rule["os"].toObject()["name"].toString() == settings->getPlatform()) {
                        allow = true;
                    }
                }

                // Make exclusions from allow-list
                if (rule["action"].toString() == "disallow") {
                    if (rule["os"].toObject()["name"].toString() == settings->getPlatform()) {
                        allow = false;
                    }
                }
            }
        }
        if (!allow) continue; // Go to next lib entry, if this are disallowed

        // Check for natives entry: <package>/<name>-<version>-<native_string>
        QString nativesSuffix = library["natives"].toObject()[settings->getPlatform()].toString();
        nativesSuffix.replace("${arch}", settings->getArch());
        if (!nativesSuffix.isEmpty()) {
            fileSuffix += "-" + nativesSuffix + ".jar";
        } else {
            fileSuffix += ".jar";
        }

        QFile* file = new QFile(filePrefix + "/" + fileSuffix);
        if (!file->exists()) {
            addTarget(QUrl("https://libraries.minecraft.net/" + fileSuffix), file->fileName());
        }

        delete file;

    }

    assets = json.object()["assets"].toString();

    return true;
}

bool UpdateDialog::checkAssets() {
    ui->log->appendPlainText("\n3. Проверка состояния игровых ресурсов...");

    // assets are defined on check-libs stage
    if (assets.isEmpty()) {
        ui->log->appendPlainText("В конфигурационном файле отсутсвует информация о ресурсах :(");
    }

    QString filePrefix = settings->getClientDir() + "/assets";
    QFile* indexFile = new QFile(filePrefix + "/indexes/" + assets + ".json");
    if (!indexFile->exists()) {
        if (!downloadNow(QUrl("https://s3.amazonaws.com/Minecraft.Download/indexes/" + assets + ".json"), indexFile->fileName())) return false;
    }

    if (!indexFile->open(QIODevice::ReadOnly)) {
        ui->log->appendPlainText("Не удалось открыть индексный файл :(");
        return false;
    }

    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(indexFile->readAll(), &error);
    indexFile->close();
    delete indexFile;

    if (!error.error == QJsonParseError::NoError) {
        ui->log->appendPlainText("Ошибка разбора индексного файла :(");
        return false;
    }

    QJsonObject objects = json.object()["objects"].toObject();
    float progress = 0;
    for (QJsonObject::iterator objit = objects.begin(), end = objects.end(); objit != end; ++objit) {
        progress++;
        ui->progressBar->setValue((progress / (objects.count() - 1)) * 10 + 60 );
        QJsonObject object = (*objit).toObject();

        QFile* assetFile = new QFile(filePrefix + "/objects/"
                                     + object["hash"].toString().mid(0, 2) + "/"
                                     + object["hash"].toString());
        if (!assetFile->exists()) {
            addAssetTarget(
                        QUrl("http://resources.download.minecraft.net/"
                             + object["hash"].toString().mid(0, 2)+ "/"
                             + object["hash"].toString()),
                        assetFile->fileName(), objit.key(), object["size"].toInt()
                        );
        }
        delete assetFile;

    }

    return true;
}

bool UpdateDialog::checkMods() {
    ui->log->appendPlainText("\n4. Проверка состояния модификаций...");

    // $ echo we need own update-server | iconv -f koi-7
    ui->log->appendPlainText("\n * ВЕ НЕЕД ОВН УПДАТЕ-СЕРЖЕР *");

    ui->progressBar->setValue(100);
    return true;
}

void UpdateDialog::checksResult(bool allGood){
    ui->log->appendPlainText("\n5. Результат:");
    if (allGood) {
        if (downloadSize > 0) {
            ui->log->appendPlainText("Требуется обновление, для выполнения нажмите кнопку \"Обновить\".");
            ui->log->appendPlainText("Необходимо загрузить "
                                     + QString::number((float)downloadSize / (1024 * 1024), 'f', 2).replace('.', ',')
                                     + " МиБ.");
            ui->updateButton->setEnabled(true);
        } else {
            ui->log->appendPlainText("Обновление не требуется!");
        }
    } else {
        ui->log->appendPlainText("Во время проверки клиента возникли проблемы. Обновление невозможно :(");
    }
}

bool UpdateDialog::isLatestVersionKnown(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray rawResponce = reply->readAll();
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(rawResponce, &error);

        // Check for incorrect JSON
        if (error.error == QJsonParseError::NoError) {
            QJsonObject responce = json.object();

            // Check for error in server answer
            if (responce["error"].toString() != "") {
                // Error in answer handler
                ui->log->appendPlainText(responce["error"].toString());
                return false;

            } else {
                // Correct reply
                QJsonObject latest = json.object()["latest"].toObject();
                latestver = latest["release"].toString();
                if (latestver == "") return false;
                return true;
            }

        } else {
            // JSON parse error
            ui->log->appendPlainText("Ошибка бмена данными с сервером обновления. "
                                     + error.errorString() + " в позиции "
                                     + QString::number(error.offset) + ".");
            return false;
        }

    } else {
        // Connection error
        ui->log->appendPlainText("Ошибка подключения к серверу обновления. "
                                 + reply->errorString());
        return false;
    }
}

bool UpdateDialog::downloadNow(QUrl url, QString fileName) {

    ui->log->appendPlainText("Загрузка файла " + fileName.split("/").last() + "...");

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(url);
    QByteArray answer;

    QNetworkReply *reply = manager->get(request);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        answer = reply->readAll();
    } else {
        ui->log->appendPlainText("Не удалось загрузить файл: " + url.toString() + ".");
        return false;
    }
    delete manager;

    QFile* file = new QFile(fileName);
    QDir fdir = QFileInfo(fileName).absoluteDir();
    fdir.mkpath(fdir.absolutePath());

    if (!file->open(QIODevice::WriteOnly)) {
        delete file;
        ui->log->appendPlainText("Не удалось сохранить файл: " + fileName + ".");
        return false;
    }

    file->write(answer);
    file->close();
    delete file;

    return true;

}

void UpdateDialog::addTarget(QUrl url, QString fileName) {

    ui->log->appendPlainText("Файл " + fileName.split("/").last() + " добавлен в очередь загрузки.");
    downloadUrls.append(url);
    downloadNames.append(fileName);

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(url);

    QNetworkReply *reply = manager->head(request);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    downloadSize += reply->header(QNetworkRequest::ContentLengthHeader).toULongLong();
}

void UpdateDialog::addAssetTarget(QUrl url, QString fileName, QString resName, int size) {

    ui->log->appendPlainText("Ресурс " + resName + " добавлен в очередь загрузки.");

    downloadUrls.append(url);
    downloadNames.append(fileName);
    downloadSize += size;
}

void UpdateDialog::doUpdate() {
    ui->log->appendPlainText("\nОбновление клиента...");
    ui->progressBar->setValue(0);
    ui->updateButton->setEnabled(false);

    downloadedSize = 0;

    downloadReply = downloadNam->get(QNetworkRequest(downloadUrls.first()));
    ui->log->appendPlainText("Загрузка файла " + downloadNames.first() + "...");

    connect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(progress(qint64,qint64)));
    connect(downloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void UpdateDialog::progress(qint64 bytesReceived, qint64 bytesTotal)  {
    // FIXME: Need to rewrite this method!
    if (bytesReceived == bytesTotal) {
        downloadedSize += bytesReceived;
    }
    ui->progressBar->setValue( ((float)downloadedSize / downloadSize) * 100);
}

void UpdateDialog::downloadFinished()  {

    if (downloadReply->error() == QNetworkReply::NoError) {

        QFile* file = new QFile(downloadNames.first());

        QDir fdir = QFileInfo(downloadNames.first()).absoluteDir();
        fdir.mkpath(fdir.absolutePath());

        if (!file->open(QIODevice::WriteOnly)) {
            ui->log->appendPlainText("Не удалось сохранить файл. " + file->errorString());
        } else {
            file->write(downloadReply->readAll());
        }
        file->close();
        delete file;

    } else {
        ui->log->appendPlainText("Загрузка не удалась. " + downloadReply->errorString());
    }

    downloadNames.removeFirst();
    downloadUrls.removeFirst();

    if (downloadNames.isEmpty()) {
        disconnect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(progress(qint64,qint64)));
        disconnect(downloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));

        ui->log->appendPlainText("Удалось загрузить "
                                 + QString::number(ui->progressBar->value())
                                 + "% необходимых данных.");
    } else {

        disconnect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(progress(qint64,qint64)));
        disconnect(downloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));

        downloadReply = downloadNam->get(QNetworkRequest(downloadUrls.first()));
        ui->log->appendPlainText("Загрузка файла " + downloadNames.first() + "...");

        connect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(progress(qint64,qint64)));
        connect(downloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
    }
}

UpdateDialog::~UpdateDialog()
{
    delete ui;
}
