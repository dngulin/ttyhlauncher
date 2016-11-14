#include "storeinstalldialog.h"
#include "ui_storeinstalldialog.h"

#include "jsonparser.h"

StoreInstallDialog::StoreInstallDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StoreInstallDialog)
{
    ui->setupUi(this);

    settings = Settings::instance();
    logger   = Logger::logger();

    ui->log->setFont( QFontDatabase::systemFont(QFontDatabase::FixedFont) );

    installer = new FileInstaller();
    installer->moveToThread(&installThread);

    // Installer sonnections
    connect(&installThread, &QThread::finished,
            installer, &QObject::deleteLater);

    connect(this, &StoreInstallDialog::install,
            installer, &FileInstaller::doInstall);

    connect(installer, &FileInstaller::progress,
            ui->progressBar, &QProgressBar::setValue);

    connect(installer, &FileInstaller::installFailed,
            this, &StoreInstallDialog::installError);

    connect(installer, &FileInstaller::finished,
            this, &StoreInstallDialog::installFinished);

    installThread.start();

    // Button connections
    connect(ui->installButton, &QPushButton::clicked,
            this, &StoreInstallDialog::installClicked);

    connect(ui->cancelButton, &QPushButton::clicked,
            this, &StoreInstallDialog::cancelClicked);

    setupLocalStoreVersions();
    setupPrefixes();

    installing = false;
}

StoreInstallDialog::~StoreInstallDialog()
{
    if (installing)
    {
        installer->cancel();
        installing = false;
    }

    installThread.quit();
    installThread.wait();

    delete ui;
}

void StoreInstallDialog::log(const QString &line, bool hidden)
{
    logger->appendLine(tr("StoreInstallDialog"), line);
    if (!hidden)
    {
        ui->log->appendPlainText(line);
    }
}

void StoreInstallDialog::setupLocalStoreVersions()
{
    log( tr("Read local prefixes...") );

    JsonParser parser;
    QString storeDirPath = settings->loadStoreDirPath();
    if ( parser.setJsonFromFile(storeDirPath + "/prefixes.json") )
    {
        if ( parser.hasPrefixesList() )
        {
            QStringList prefixes = parser.getPrefixesList().keys();
            foreach (QString prefix, prefixes)
            {
                getLocalPrefixVersions(prefix);
            }
            log( tr("Local prefixes list ready") );
        }
        else
        {
            log( tr("Local prefixes list not defined in index!") );
        }
    }
    else
    {
        log( tr("Can't read local prefixes!") );
        log( parser.getParserError() );
    }
}

void StoreInstallDialog::getLocalPrefixVersions(const QString &prefix)
{
    log( tr("Read local versions for prefix %1...").arg(prefix) );

    JsonParser parser;
    QString prefixPath = settings->loadStoreDirPath() + "/" + prefix;
    if ( parser.setJsonFromFile(prefixPath + "/versions/versions.json") )
    {
        if ( parser.hasVersionList() )
        {
            QStringList versions = parser.getReleaseVersonList();
            foreach (QString version, versions)
            {
                ui->versionCombo->addItem(prefix + "/" + version);
            }
        }
        else
        {
            log( tr("Local version list not defined in index!") );
        }
    }
    else
    {
        log( tr("Can't read local versions for prefix %1!").arg(prefix) );
        log( parser.getParserError() );
    }
}

void StoreInstallDialog::setupPrefixes()
{
    ui->prefixCombo->addItems( settings->getClientCaptions() );
    ui->prefixCombo->setCurrentIndex( settings->loadActiveClientID() );
}

void StoreInstallDialog::setInteractable(bool state)
{
    ui->versionCombo->setEnabled(state);
    ui->prefixCombo->setEnabled(state);
    ui->installButton->setEnabled(state);
}

void StoreInstallDialog::installClicked()
{
    installList.clear();
    ui->progressBar->setValue(0);
    ui->log->clear();

    QString version = ui->versionCombo->currentText();
    if ( version.isEmpty() )
    {
        log( tr("Error: version for install not selected!") );
        return;
    }

    int id = ui->prefixCombo->currentIndex();
    QString prefix = settings->getClientName(id);
    if (prefix == "unknown")
    {
        log( tr("Error: installation prefix not selected!") );
        return;
    }

    storeDir  = settings->loadStoreDirPath();
    clientDir = settings->getBaseDir() + "/client_" + prefix;

    QStringList versionData = version.split('/');
    storePrefix = versionData.first();
    storeVersion = versionData.last();

    QString beginMsg = tr("Try to install local version %1 to prefix %2");
    log( beginMsg.arg(version).arg(prefix) );

    QString dataIdxPath = storeDir + "/" + version + "/data.json";

    JsonParser dataParser;
    if ( !dataParser.setJsonFromFile(dataIdxPath) )
    {
        log( tr("Error: can't read data.json!") );
        log( dataParser.getParserError() );
        return;
    }

    // Prepare files

    if ( dataParser.hasJarFileInfo() )
    {
        prepareVersion( dataParser.getJarFileInfo().hash );
    }
    else
    {
        log( tr("Error: jar file does not described in data.json!") );
    }

    if ( dataParser.hasLibsFileInfo() )
    {
        prepareLibararies( dataParser.getLibsFileInfo() );
    }
    else
    {
        log( tr("Error: libraries are not described in data.json") );
    }

    if ( dataParser.hasAddonsFilesInfo() )
    {
        prepareAddons( dataParser.getAddonsFilesInfoHashMap() );
    }
    else
    {
        log( tr("Error: addons are not described in data.json") );
    }

    prepareAssets();

    log( tr("Begin copy files...") );
    setInteractable(false);
    emit install(installList);
    installing = true;
}

void StoreInstallDialog::prepareVersion(const QString &jarHash)
{
    log( tr("Prepare local version...") );

    QString storeVerDir  = storeDir + "/" + storePrefix + "/" + storeVersion;
    QString clientVerDir = clientDir + "/versions/" + storeVersion;

    QStringList files;
    files << storeVersion + ".jar" << storeVersion + ".json" << "data.json";

    foreach (QString fileName, files)
    {
        InstallInfo info;
        info.hash    = (fileName == storeVersion + ".jar") ? jarHash : "0";
        info.srcPath = storeVerDir + "/" + fileName;
        info.path    = clientVerDir + "/" + fileName;
        installList.append(info);
    }
}

void StoreInstallDialog::prepareLibararies(const QList<FileInfo> &libs)
{
    log( tr("Prepare libraries...") );

    QString storeLibDir = storeDir + "/libraries/";
    QString clientLibDir = settings->getLibsDir() + "/";

    foreach(FileInfo lib, libs)
    {
        InstallInfo info;
        info.hash    = lib.hash;
        info.srcPath = storeLibDir + lib.name;
        info.path    = clientLibDir + lib.name;
        installList.append(info);
    }
}

void StoreInstallDialog::prepareAddons(const QHash<QString, FileInfo> &addons)
{
    log( tr("Prepare addons...") );

    QString storeVerDir  = storeDir + "/" + storePrefix + "/" + storeVersion;
    QString clientPrefixDir = clientDir + "/prefixes/" + storeVersion;

    foreach ( FileInfo addon, addons.values() )
    {
        InstallInfo info;
        info.srcPath = storeVerDir + "/files/" + addon.name;
        info.path    = clientPrefixDir + "/" + addon.name;
        if (!addon.isMutable)
        {
            info.hash = addon.hash;
        }
        installList.append(info);
    }

    JsonParser prefixParser;
    QString instIdxPath = clientPrefixDir + "/installed_data.json";

    bool installed = prefixParser.setJsonFromFile(instIdxPath) ;
    if ( installed && prefixParser.hasAddonsFilesInfo() )
    {
        log( tr("Read installed prefix...") );
        QList<FileInfo> oldAddons = prefixParser.getAddonsFilesInfo();
        foreach (FileInfo oldAddon, oldAddons)
        {
            if (!addons.contains(oldAddon.name))
            {
                InstallInfo obsoleteInfo;
                obsoleteInfo.action = InstallInfo::Delete;
                obsoleteInfo.path   = clientPrefixDir + "/" + oldAddon.name;
                installList.append(obsoleteInfo);
            }
        }
    }

    InstallInfo installedInfo;
    installedInfo.hash    = "0";
    installedInfo.srcPath = storeVerDir + "/data.json";
    installedInfo.path    = instIdxPath;
    installList.append(installedInfo);
}

void StoreInstallDialog::prepareAssets()
{
    QString storeVerDir  = storeDir + "/" + storePrefix + "/" + storeVersion;
    QString versionIdxPath = storeVerDir + "/" + storeVersion + ".json";

    JsonParser indexParser;
    if ( !indexParser.setJsonFromFile(versionIdxPath) )
    {
        log( tr("Error: can't parse index '%1'!").arg(versionIdxPath) );
        log( indexParser.getParserError() );
        return;
    }

    if ( !indexParser.hasAssetsVersion() )
    {
        QString msg = tr("Error: '%1' does not contains assets version!");
        log( msg.arg(versionIdxPath) );
        return;
    }

    QString assetsVer = indexParser.getAssetsVesrsion();
    QString storeAssetsDir = storeDir + "/assets";
    QString clientAssetsDir = settings->getAssetsDir();

    InstallInfo assetsIdxInfo;
    assetsIdxInfo.hash = "0";
    assetsIdxInfo.srcPath = storeAssetsDir + "/indexes/" + assetsVer + ".json";
    assetsIdxInfo.path = clientAssetsDir + "/indexes/" + assetsVer + ".json";
    installList.append(assetsIdxInfo);

    QString assetsIdxPath = storeAssetsDir + "/indexes/" + assetsVer + ".json";
    JsonParser assetsParser;
    if ( !assetsParser.setJsonFromFile(assetsIdxPath) )
    {
        log( tr("Error: can't parse index '%1'!").arg(assetsIdxPath) );
        log( assetsParser.getParserError() );
        return;
    }

    if ( !assetsParser.hasAssetsList() )
    {
        QString msg = tr("Error: '%1' does not contains assets version!");
        log( msg.arg(assetsIdxPath) );
        return;
    }

    QList<FileInfo> assets = assetsParser.getAssetsList();
    foreach(FileInfo asset, assets)
    {
        InstallInfo assetInfo;
        assetInfo.hash    = asset.hash;
        assetInfo.srcPath = storeAssetsDir + "/objects/" + asset.name;
        assetInfo.path    = clientAssetsDir + "/objects/" + asset.name;
        installList.append(assetInfo);
    }
}


void StoreInstallDialog::cancelClicked()
{
    if (installing)
    {
        installer->cancel();
        installList.clear();
        installing = false;
        setInteractable(true);
    }
    else
    {
        this->close();
    }
}

void StoreInstallDialog::installError(const InstallInfo &info)
{
    if (info.action == InstallInfo::Delete)
    {
        log( tr("Error: can't delete file: %1").arg(info.path) );
    }
    else if (info.action == InstallInfo::Update)
    {
        log( tr("Error: can't update file: %1").arg(info.path) );
    }
}

void StoreInstallDialog::installFinished()
{
    log( tr("Installation finished!") );
    setInteractable(true);
    installList.clear();
    installing = false;
}
