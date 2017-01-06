#include "updatedialog.h"
#include "ui_updatedialog.h"

#include "settings.h"
#include "logger.h"
#include "util.h"

UpdateDialog::UpdateDialog(QString displayMessage, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateDialog)
{
    ui->setupUi(this);

    checker = new HashChecker();
    checker->moveToThread(&checkThread);

    // Check files sonnection
    connect(&checkThread, &QThread::finished, checker, &QObject::deleteLater);

    connect(this, &UpdateDialog::checkFiles, checker, &HashChecker::checkFiles);

    connect(checker, &HashChecker::progress,
            ui->progressBar, &QProgressBar::setValue);

    connect(checker, &HashChecker::verificationFailed,
            this, &UpdateDialog::addToFetchList);

    connect(checker, &HashChecker::finished,
            this, &UpdateDialog::checkFinished);

    checkThread.start();

    settings = Settings::instance();
    logger = Logger::logger();

    ui->clientCombo->addItems( settings->getClientCaptions() );
    ui->clientCombo->setCurrentIndex( settings->loadActiveClientID() );

    // Old-style connection for overloaded signals
    connect( ui->clientCombo, SIGNAL(activated(int)),
             settings, SLOT(saveActiveClientID(int)));

    connect( ui->clientCombo, SIGNAL(activated(int)),
             this, SLOT(clientChanged()));

    ui->log->setFont( QFontDatabase::systemFont(QFontDatabase::FixedFont) );
    ui->log->setPlainText(displayMessage);

    // Buttons
    connect(ui->updateButton, &QPushButton::clicked,
            this, &UpdateDialog::updateClicked);

    connect(ui->cancelButton, &QPushButton::clicked,
            this, &UpdateDialog::cancelClicked);

    // Downloading conncetions
    connect(&fileFetcher, &FileFetcher::filesFetchResult,
            this, &UpdateDialog::updateComplete);

    connect(&fileFetcher, &FileFetcher::filesFetchProgress,
            ui->progressBar, &QProgressBar::setValue);

    setState(CanCheck);

    if (ui->clientCombo->count() == 0)
    {
        ui->updateButton->setEnabled(false);
        log( tr("Error! No clients available for check.") );
    }
}

void UpdateDialog::log(const QString &line, bool hidden)
{
    logger->appendLine(tr("UpdateDialog"), line);
    if (!hidden)
    {
        ui->log->appendPlainText(line);
    }
}

void UpdateDialog::error(const QString &line)
{
    log( tr("Error! %1").arg(line) );
    setState(CanCheck);
}

void UpdateDialog::setInteractable(bool state)
{
    ui->clientCombo->setEnabled(state);
    ui->updateButton->setEnabled(state);
    ui->cancelButton->setEnabled(!state);
}

void UpdateDialog::resetUpdateData()
{
    fileFetcher.reset();
    checker->cancel();
    removeList.clear();
    checkList.clear();
}

void UpdateDialog::setState(UpdaterState newState)
{
    ui->progressBar->setValue(0);
    state = newState;

    switch (state)
    {
    case CanCheck:
        resetUpdateData();
        ui->updateButton->setText( tr("Check for updates") );
        setInteractable(true);

        break;

    case Checking:
        setInteractable(false);
        doCheck();
        break;

    case CanUpdate:
        ui->updateButton->setText( tr("Update client") );
        setInteractable(true);
        break;

    case Updating:
        setInteractable(false);
        doUpdate();
        break;

    case CanClose:
        ui->updateButton->setText( tr("Close updater") );
        setInteractable(true);
        break;

    default:
        log( tr("Error! Updater tool swithed to unknown state!") );
        break;
    }
}

void UpdateDialog::clientChanged()
{
    setState(CanCheck);
    ui->log->setPlainText( tr("Select client, then press 'Check' button") );
}

void UpdateDialog::updateClicked()
{
    switch (state)
    {
    case CanCheck:
        setState(Checking);
        break;

    case CanUpdate:
        setState(Updating);
        break;

    case CanClose:
        this->close();
        break;

    default:
        log( tr("Error! Update button clicked in not allowed state!") );
        setState(CanCheck);
        break;
    }
}

void UpdateDialog::cancelClicked()
{
    switch (state)
    {
    case Checking:
    case Updating:
        log( tr("The operation is canceled!") );
        setState(CanCheck);
        break;

    default:
        log( tr("Error! Cancel button clicked in not allowed state!") );
        setState(CanCheck);
        break;
    }
}

// Checking sequence

void UpdateDialog::doCheck()
{
    int index = settings->loadActiveClientID();
    QString client = settings->getClientName(index);

    clientVersion = settings->loadClientVersion();

    ui->log->clear();
    log( tr("Checking for updates... ") );
    log( tr("Client: %1, version %2.").arg(client).arg(clientVersion) );

    if (clientVersion == "latest")
    {
        log( tr("Requesting latest client version...") );
        connect(&dataFetcher, &DataFetcher::finished,
                this, &UpdateDialog::versionListReceived);

        dataFetcher.makeGet( settings->getVersionsUrl() );
    }
    else
    {
        requestVersionIndex();
    }
}

void UpdateDialog::versionListReceived(bool result)
{
    disconnect(&dataFetcher, &DataFetcher::finished,
               this, &UpdateDialog::versionListReceived);

    if (!result)
    {
        error( tr("Latest version does not received.") );
        return;
    }

    JsonParser versionsParser;
    if ( !versionsParser.setJson( dataFetcher.getData() ) )
    {
        log(versionsParser.getParserError(), true);
        error( tr("Inavlid reply.") );
        return;
    }

    if ( !versionsParser.hasLatestReleaseVersion() )
    {
        error( tr("Reply does not contains 'latest' version.") );
        return;
    }

    clientVersion = versionsParser.getLatestReleaseVersion();
    log( tr("Client version received: %1.").arg(clientVersion) );

    requestVersionIndex();
}

void UpdateDialog::requestVersionIndex()
{
    QString versionUrlPrefix = settings->getVersionUrl(clientVersion);
    QString versionUrl = versionUrlPrefix + clientVersion + ".json";

    log( tr("Requesting actual version index...") );

    connect(&dataFetcher, &DataFetcher::finished,
            this, &UpdateDialog::versionIndexReceived);

    dataFetcher.makeGet(versionUrl);
}

void UpdateDialog::versionIndexReceived(bool result)
{
    disconnect(&dataFetcher, &DataFetcher::finished,
               this, &UpdateDialog::versionIndexReceived);

    if (!result)
    {
        error( tr("Can't get version index.") );
        return;
    }

    QByteArray versionIndex = dataFetcher.getData();
    if ( !versionParser.setJson(versionIndex) )
    {
        log(versionParser.getParserError(), true);
        error( tr("Can't read version index.") );
        return;
    }

    QString hash = HashChecker::getDataHash(versionIndex);

    QString pathPtrn = "%1/%2/%2.json";
    QString versionsDir = settings->getVersionsDir();
    QString path = pathPtrn.arg(versionsDir).arg(clientVersion);

    if ( !HashChecker::isFileHashValid(path, hash) )
    {
        QString versionUrlPrefix = settings->getVersionUrl(clientVersion);

        FileInfo indexInfo;
        indexInfo.name = path;
        indexInfo.hash = hash;
        indexInfo.size = versionIndex.size();
        indexInfo.url = versionUrlPrefix + clientVersion + ".json";

        log( tr("Version index is obsolete.") );
        addToFetchList(indexInfo);
    }

    requestDataIndex();
}

void UpdateDialog::requestDataIndex()
{
    QString versionUrlPrefix = settings->getVersionUrl(clientVersion);
    QString dataUrl = versionUrlPrefix + "data.json";

    log( tr("Requesting actual data index...") );

    connect(&dataFetcher, &DataFetcher::finished,
            this, &UpdateDialog::dataIndexReceived);

    dataFetcher.makeGet(dataUrl);
}

void UpdateDialog::dataIndexReceived(bool result)
{
    disconnect(&dataFetcher, &DataFetcher::finished,
               this, &UpdateDialog::dataIndexReceived);

    if (!result)
    {
        error( tr("Can't get data index.") );
        return;
    }

    QByteArray dataIndex = dataFetcher.getData();
    if ( !dataParser.setJson(dataIndex) )
    {
        log(dataParser.getParserError(), true);
        error( tr("Can't read data index.") );
        return;
    }

    QString hash = HashChecker::getDataHash(dataIndex);

    QString pathPtrn = "%1/%2/data.json";
    QString versionsDir = settings->getVersionsDir();
    QString path = pathPtrn.arg(versionsDir).arg(clientVersion);

    if ( !HashChecker::isFileHashValid(path, hash) )
    {
        QString versionUrlPrefix = settings->getVersionUrl(clientVersion);

        FileInfo indexInfo;
        indexInfo.name = path;
        indexInfo.hash = hash;
        indexInfo.size = dataIndex.size();
        indexInfo.url = versionUrlPrefix + "data.json";

        log( tr("Data index is obsolete.") );
        addToFetchList(indexInfo);
    }

    requestAssetsIndex();
}

void UpdateDialog::requestAssetsIndex()
{
    log( tr("Requesting actual assets index...") );
    if ( !versionParser.hasAssetsVersion() )
    {
        error( tr("Assets are not described in version index.") );
        return;
    }

    QString assetsName = versionParser.getAssetsVesrsion() + ".json";
    QString assetsUrl = settings->getAssetsUrl() + "indexes/";

    connect(&dataFetcher, &DataFetcher::finished,
            this, &UpdateDialog::assetsIndexReceived);

    dataFetcher.makeGet(assetsUrl + assetsName);
}

void UpdateDialog::assetsIndexReceived(bool result)
{
    disconnect(&dataFetcher, &DataFetcher::finished,
               this, &UpdateDialog::assetsIndexReceived);

    if (!result)
    {
        error( tr("Can't get assets index.") );
        return;
    }

    QByteArray assetsIndex = dataFetcher.getData();
    if ( !assetsParser.setJson(assetsIndex) )
    {
        log(assetsParser.getParserError(), true);
        error( tr("Can't read assets index.") );
        return;
    }

    QString hash = HashChecker::getDataHash(assetsIndex);

    QString pathPtrn = "%1/indexes/%2.json";
    QString assetsDir = settings->getAssetsDir();
    QString assetsVersion = versionParser.getAssetsVesrsion();
    QString path = pathPtrn.arg(assetsDir).arg(assetsVersion);

    if ( !HashChecker::isFileHashValid(path, hash) )
    {
        QString urlPtrn = "%1indexes/%2.json";
        QString assetsUrl = settings->getAssetsUrl();
        QString url = urlPtrn.arg(assetsUrl).arg(assetsVersion);

        FileInfo indexInfo;
        indexInfo.name = path;
        indexInfo.hash = hash;
        indexInfo.size = assetsIndex.size();
        indexInfo.url = url;

        log( tr("Assets index is obsolete.") );
        addToFetchList(indexInfo);
    }

    processIndexesData();
}

void UpdateDialog::processIndexesData()
{
    // I. JAR
    log( tr("Append main JAR to check list...") );
    if ( !dataParser.hasJarFileInfo() )
    {
        error( tr("Main JAR does not described in data index.") );
        return;
    }

    FileInfo jar;

    QString indexDir = settings->getVersionsDir() + "/" + clientVersion + "/";
    jar.name = indexDir + clientVersion + ".jar";
    jar.hash = dataParser.getJarFileInfo().hash;
    jar.size = dataParser.getJarFileInfo().size;
    jar.url = settings->getVersionUrl(clientVersion) + clientVersion + ".jar";

    checkList.append(jar);

    // II. LIBS
    log( tr("Append libraries to checklist...") );
    if ( !versionParser.hasLibraryList() )
    {
        error( tr("Libraries are not described in data index.") );
        return;
    }

    QString libDir = settings->getLibsDir() + "/";
    QString libUrl = settings->getLibsUrl();

    QList<LibraryInfo> liblist = versionParser.getLibraryList();
    foreach (LibraryInfo entry, liblist)
    {
        QString lib = entry.name;

        if ( !dataParser.hasLibFileInfo(lib) )
        {
            error( tr("Data index does not contain library: %1.").arg(lib) );
            return;
        }

        FileInfo fileInfo = dataParser.getLibFileInfo(lib);
        fileInfo.name = libDir + lib;
        fileInfo.url = libUrl + lib;

        checkList.append(fileInfo);
    }

    // III. ADDONS
    if ( !dataParser.hasAddonsFilesInfo() )
    {
        error( tr("Addons are not described in data index.") );
        return;
    }

    log( tr("Append addons to check-list...") );
    QString clientPrefix = settings->getClientPrefix(clientVersion) + "/";
    QString addonsUrl = settings->getVersionUrl(clientVersion) + "files/";

    QList<FileInfo> addons = dataParser.getAddonsFilesInfo();
    foreach (FileInfo addon, addons)
    {
        QString shortName = addon.name;
        addon.name = clientPrefix + shortName;
        addon.url = addonsUrl + shortName;

        checkList.append(addon);
    }

    log( tr("Checking for obsolete addons...") );
    QString installedIndex = clientPrefix + "installed_data.json";

    JsonParser installedParser;
    if ( installedParser.setJsonFromFile(installedIndex) )
    {
        typedef QHash<QString, FileInfo> mapInfo;

        mapInfo newFiles = dataParser.getAddonsFilesInfoHashMap();
        mapInfo oldFiles = installedParser.getAddonsFilesInfoHashMap();

        foreach ( QString oldFile, oldFiles.keys() )
        {
            if ( !newFiles.contains(oldFile) )
            {
                removeList.append(oldFile);
            }
        }
    }
    else
    {
        QString msg = tr("Can't read installed data index: %1.");
        log(msg.arg(installedIndex), true);
        log(installedParser.getParserError(), true);
    }

    // IV. ASSETS
    log( tr("Append assets files to check list...") );
    if ( !assetsParser.hasAssetsList() )
    {
        error( tr("Assets list are not described in index.") );
        return;
    }

    QList<FileInfo> assets = assetsParser.getAssetsList();

    QString assetsDir = settings->getAssetsDir() + "/objects/";
    QString assetsUrl = settings->getAssetsUrl() + "objects/";

    foreach (FileInfo asset, assets)
    {
        QString shortName = asset.name;
        asset.name = assetsDir + shortName;
        asset.url = assetsUrl + shortName;

        checkList.append(asset);
    }

    log( tr("Checking files...") );
    emit checkFiles(checkList, false);
}

void UpdateDialog::checkFinished()
{
    log( tr("Done!") );

    bool need_fetch = fileFetcher.getCount() > 0;
    bool need_remove = !removeList.empty();

    if (need_fetch || need_remove)
    {
        ui->log->appendPlainText("");

        if (need_remove)
        {
            foreach (QString file, removeList)
            {
                log( tr("File: %1 will be removed.").arg(file) );
            }
        }

        if (need_fetch)
        {
            int count = fileFetcher.getCount();

            double size = double( fileFetcher.getFetchSize() ) / 1024;
            QString suffix = tr("KiB");

            if (size > 1024 * 1024)
            {
                size = size / (1024 * 1024);
                suffix = tr("GiB");
            }
            else if (size > 1024)
            {
                size = size / 1024;
                suffix = tr("MiB");
            }

            QString downloadCount = QString::number(count);
            QString downloadSize = QString::number(size, 'f', 2);

            log( tr("Need to download %1 files.").arg(downloadCount) );
            log( tr("Download size %1 %2.").arg(downloadSize).arg(suffix) );
        }

        log( tr("Press update button to continue.") );
        setState(UpdateDialog::CanUpdate);
    }
    else
    {
        log( tr("Client checking completed, no need update.") );
        setState(UpdateDialog::CanClose);
    }
}

void UpdateDialog::doUpdate()
{
    ui->log->appendPlainText("");

    if (fileFetcher.getCount() > 0)
    {
        log( tr("Downloading files...") );
        fileFetcher.fetchFiles();
    }
    else
    {
        updateComplete(true);
    }
}

void UpdateDialog::updateComplete(bool result)
{
    if (result)
    {
        QString clientDir
            = settings->getClientPrefix(clientVersion) + "/";
        QString versionDir
            = settings->getVersionsDir() + "/" + clientVersion + "/";

        // Remove obsolete files
        if ( !removeList.empty() )
        {
            log( tr("Removing obsolete files...") );

            int total = removeList.size();
            int current = 1;

            foreach (QString entry, removeList)
            {
                log(tr("Removing: ") + entry);
                QFile::remove(clientDir + entry);

                current++;
                ui->progressBar->setValue( int(float(current) * 100 / total) );
            }

            removeList.clear();
        }

        // Update installed_data index
        QFile::remove(clientDir + "installed_data.json");
        QDir(clientDir).mkpath(clientDir);
        QFile::copy(versionDir + "data.json",
                    clientDir + "installed_data.json");

        ui->log->appendPlainText("");
        log( tr("Update complete!") );
    }
    else
    {
        ui->log->appendPlainText("");
        log( tr("Update not completed. Some files was not downloaded.") );
    }
    setState(CanClose);
}

void UpdateDialog::addToFetchList(const FileInfo fileInfo)
{
    fileFetcher.add(fileInfo.url, fileInfo.name, fileInfo.size);
}

UpdateDialog::~UpdateDialog()
{
    checkThread.quit();
    checkThread.wait();

    delete ui;
}
