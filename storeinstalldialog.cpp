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

    setupLocalStoreVersions();
    setupPrefixes();
}

StoreInstallDialog::~StoreInstallDialog()
{
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
    QString versionName = version.split('/').last();

    int id = ui->prefixCombo->currentIndex();
    QString prefix = settings->getClientName(id);
    if (prefix == "unknown")
    {
        log( tr("Error: installation prefix not selected!") );
        return;
    }
    QString clientDir = settings->getBaseDir() + "/client_" + prefix;
    QString clientVersionPath = clientDir + "/versions/" + versionName;
    QString clientPrefixPath = clientDir + "/prefixes/" + versionName;

    QString beginMsg = tr("Try to install local version %1 to prefix %2");
    log( beginMsg.arg(version).arg(prefix) );

    QString storeDirPath = settings->loadStoreDirPath();
    QString storeVersionPath = storeDirPath + "/" + version;
    QString indexPath = storeVersionPath + "/data.json";

    JsonParser parser;
    if ( !parser.setJsonFromFile(indexPath) )
    {
        log( tr("Error: can't read data.json!") );
        log( parser.getParserError() );
        return;
    }

    if ( !parser.hasJarFileInfo() )
    {
        log( tr("Error: jar file not described in data.json!") );
        return;
    }

    log( tr("Prepare main files...") );

    // Main JAR
    InstallInfo jarInfo;
    jarInfo.hash    = parser.getJarFileInfo().hash;
    jarInfo.srcPath = storeVersionPath + "/" + versionName + ".jar";
    jarInfo.path    = clientVersionPath + "/" + versionName + ".jar";
    installList.append(jarInfo);

    // Index JSON
    InstallInfo indexJsonInfo;
    indexJsonInfo.hash    = "force";
    indexJsonInfo.srcPath = storeVersionPath + "/" + versionName + ".json";
    indexJsonInfo.path    = clientVersionPath + "/" + versionName + ".json";
    installList.append(indexJsonInfo);

    // Data JSON
    InstallInfo dataJsonInfo;
    dataJsonInfo.hash    = "force";
    dataJsonInfo.srcPath = storeVersionPath + "/data.json";
    dataJsonInfo.path    = clientVersionPath + "/data.json";
    installList.append(dataJsonInfo);

    // Libararies
    if ( parser.hasLibsFileInfo() )
    {
        log( tr("Prepare libraries...") );

        QString localLibsDir = settings->loadStoreDirPath() + "libraries/";
        QString storeLibsDir = settings->getLibsDir() + "/";

        QList<FileInfo> libs = parser.getLibsFileInfo();
        foreach(FileInfo lib, libs)
        {
            InstallInfo libInfo;
            libInfo.hash    = lib.hash;
            libInfo.srcPath = storeLibsDir + lib.name;
            libInfo.path    = localLibsDir + lib.name;

            installList.append(libInfo);
        }
    }

    // Files
    QHash<QString, FileInfo> addonsMap;
    if ( parser.hasAddonsFilesInfo() )
    {
        log( tr("Prepare addons...") );
        addonsMap = parser.getAddonsFilesInfoHashMap();
        foreach ( FileInfo addon, addonsMap.values() )
        {
            InstallInfo addonInfo;
            addonInfo.srcPath = storeVersionPath + "/files/" + addon.name;
            addonInfo.path    = clientPrefixPath + "/" + addon.name;

            if (!addon.isMutable)
            {
                addonInfo.hash = addon.hash;
            }

            installList.append(addonInfo);
        }
    }

    // Installed files
    JsonParser prefixParser;
    QString installedData = clientPrefixPath + "/installed_data.json";

    bool installed = prefixParser.setJsonFromFile(installedData) ;
    if ( installed && prefixParser.hasAddonsFilesInfo() )
    {
        log( tr("Read installed prefix...") );
        QList<FileInfo> oldFiles = prefixParser.getAddonsFilesInfo();
        foreach (FileInfo oldFile, oldFiles)
        {
            if (!addonsMap.contains(oldFile.name))
            {
                InstallInfo obsoleteInfo;
                obsoleteInfo.action = InstallInfo::Delete;
                obsoleteInfo.path   = clientPrefixPath + "/" + oldFile.name;
                installList.append(obsoleteInfo);
            }
        }
    }

    // Assets?

    // InstalledData JSON
    InstallInfo installedInfo;
    installedInfo.hash    = "force";
    installedInfo.srcPath = storeVersionPath + "/data.json";
    installedInfo.path    = installedData;
    installList.append(installedInfo);

    log( tr("Begin installation...") );
    setInteractable(false);
    emit install(installList);
}

void StoreInstallDialog::cancelClicked()
{

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
    log( tr("Installation finished") );
    setInteractable(true);
    installList.clear();
}
