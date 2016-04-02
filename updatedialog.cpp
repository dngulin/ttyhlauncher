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

    fetcher = new FileFetcher(this);

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

    connect(ui->cancelButton, &QPushButton::clicked,
            checker, &HashChecker::cancel);

    checkThread.start();

    // Fecth files conections
    connect(fetcher, &FileFetcher::filesFetchProgress,
            ui->progressBar, &QProgressBar::setValue);

    connect(fetcher, &FileFetcher::filesFetchFinished,
            this, &UpdateDialog::updateFinished);

    settings = Settings::instance();
    logger = Logger::logger();

    ui->clientCombo->addItems( settings->getClientsNames() );
    ui->clientCombo->setCurrentIndex( settings->loadActiveClientId() );

    // Old-style connection for overloaded signals
    connect( ui->clientCombo, SIGNAL( activated(int) ),
             settings, SLOT( saveActiveClientId(int) ) );

    connect( ui->clientCombo, SIGNAL( activated(int) ),
             this, SLOT( clientChanged() ) );

    ui->log->setFont( QFontDatabase::systemFont(QFontDatabase::FixedFont) );
    ui->log->setPlainText(displayMessage);

    // Buttons
    connect(ui->updateButton, &QPushButton::clicked,
            this, &UpdateDialog::updateClicked);

    connect(ui->cancelButton, &QPushButton::clicked,
            this, &UpdateDialog::cancelClicked);

    // Checking connections
    connect(&versionsFetcher, &DataFetcher::finished,
            this, &UpdateDialog::versionListRequested);

    setState(CanCheck);

    if (ui->clientCombo->count() == 0)
    {
        ui->updateButton->setEnabled(false);
        log( tr("Error! No clients available for check.") );
    }
}

void UpdateDialog::log(const QString &line, bool hidden)
{
    logger->append(tr("UpdateDialog"), line + "\n");
    if (!hidden)
    {
        ui->log->appendPlainText(line);
    }
}

void UpdateDialog::error(const QString &line)
{
    log(line);
    setState(CanClose);
}

void UpdateDialog::setInteractable(bool state)
{
    ui->clientCombo->setEnabled(state);
    ui->updateButton->setEnabled(state);
}

void UpdateDialog::setState(UpdaterState newState)
{
    ui->progressBar->setValue(0);
    state = newState;

    switch (state)
    {
    case CanCheck:
        fetcher->cancel();

        removeList.clear();
        checkList.clear();
        checkInfo.clear();

        ui->updateButton->setText( tr("Check for updates") );
        setInteractable(true);

        break;

    case Checking:
        setInteractable(false);
        checkStart();
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
    case CanCheck:
    case CanUpdate:
    case CanClose:
        this->close();
        break;

    case Checking:
    case Updating:
        setState(CanCheck);
        break;

    default:
        log( tr("Error! Cancel button clicked in not allowed state!") );
        setState(CanCheck);
        break;
    }
}

// Checking sequence

void UpdateDialog::checkStart()
{
    int clientId = settings->loadActiveClientId();
    QString clientString = settings->getClientStrId(clientId);

    clientVersion = settings->loadClientVersion();

    log( tr("Checking for updates... ") );
    log(tr("Client: ") + clientString + tr(", version: ") + clientVersion);

    if (clientVersion == "latest")
    {
        log( tr("Requesting latest client version...") );
        versionsFetcher.makeGet( settings->getVersionsUrl() );
    }
    else
    {
        updateVersionIndex();
    }
}

void UpdateDialog::versionListRequested(bool result)
{
    if (!result)
    {
        error( tr("Error! Latest version does not received.") );
        return;
    }

    JsonParser versionsParser;
    if ( !versionsParser.setJson( versionsFetcher.getData() ) )
    {
        log(versionsParser.getParserError(), true);
        error( tr("Error! Inavlid reply.") );
        return;
    }

    if ( !versionsParser.hasLatestReleaseVersion() )
    {
        error( tr("Error! Reply does not contains 'latest' version.") );
        return;
    }

    clientVersion = versionsParser.getLatestReleaseVersion();
    log(tr("Client version received: ") + clientVersion);

    updateVersionIndex();
}

void UpdateDialog::updateVersionIndex()
{
    QString versionsDir = settings->getVersionsDir();
    QString versionFilePrefix = versionsDir + "/" + clientVersion + "/";
    QString versionUrlPrefix = settings->getVersionUrl(clientVersion);

    QUrl versionUrl(versionUrlPrefix + clientVersion + ".json");
    QString versionFile = versionFilePrefix + clientVersion + ".json";

    QUrl dataUrl(versionUrlPrefix + "data.json");
    QString dataFile = versionFilePrefix + "data.json";

    log( tr("Requesting actual version indexes...") );

    FileFetcher indexFetcher;
    indexFetcher.add(versionUrl, versionFile);
    indexFetcher.add(dataUrl, dataFile);

    connect(&indexFetcher, &FileFetcher::filesFetchResult,
            this, &UpdateDialog::versionIndexUpdated);

    indexFetcher.fetchFiles();
}

void UpdateDialog::versionIndexUpdated(bool result)
{
    if (!result)
    {
        error( tr("Error! Can't update version indexes.") );
        return;
    }

    processClientFiles();
}

void UpdateDialog::processClientFiles()
{
    QString indexDir = settings->getVersionsDir() + "/" + clientVersion + "/";
    QString indexName = indexDir + clientVersion + ".json";
    QString dataName = indexDir + "data.json";

    if ( !versionParser.setJsonFromFile(indexName) )
    {
        log(versionParser.getParserError(), true);
        error( tr("Error! Can't read version index.") );
        return;
    }

    if ( !dataParser.setJsonFromFile(dataName) )
    {
        error( tr("Error! Can't read data index.") );
        log(dataParser.getParserError(), true);
        return;
    }

    // Check files
    QString name, hash, url;
    quint64 size;

    // I. JAR
    log( tr("Append main JAR to check list...") );
    if ( !dataParser.hasJarFileInfo() )
    {
        error( tr("Error! Main JAR does not described in data index.") );
        return;
    }

    name = indexDir + clientVersion + ".jar";
    hash = dataParser.getJarFileInfo().hash;
    size = dataParser.getJarFileInfo().size;
    url = settings->getVersionUrl(clientVersion) + clientVersion + ".jar";
    addToCheckList(name, hash, size, url);

    // II. LIBS
    log( tr("Append libraries to checklist...") );
    if ( !versionParser.hasLibraryList() )
    {
        error( tr("Error! Libraries are not described in data index.") );
        return;
    }

    QString libDir = settings->getLibsDir() + "/";
    QString libUrl = settings->getLibsUrl();

    QList<LibraryInfo> liblist = versionParser.getLibraryList();
    foreach (LibraryInfo libInfo, liblist)
    {
        QString lib = libInfo.name;

        if ( !dataParser.hasLibFileInfo(lib) )
        {
            error(tr("Error! Data index does not contain library: ") + lib);
            return;
        }

        name = libDir + lib;
        hash = dataParser.getLibFileInfo(lib).hash;
        size = dataParser.getLibFileInfo(lib).size;
        url = libUrl + lib;
        addToCheckList(name, hash, size, url);
    }

    // III. ADDONS
    if ( !dataParser.hasAddonsFilesInfo() )
    {
        error( tr("Error! Addons are not described in data index.") );
        return;
    }

    log( tr("Append addons to check-list...") );
    QString clientPrefix = settings->getClientPrefix(clientVersion) + "/";
    QString addonsUrl = settings->getVersionUrl(clientVersion) + "files/";

    QList<FileInfo> addons = dataParser.getAddonsFilesInfo();
    foreach (FileInfo addon, addons)
    {
        name = clientPrefix + addon.name;
        hash = addon.hash;
        size = addon.size;
        url = addonsUrl + addon.name;

        if (addon.isMutable)
        {
            hash = "mutable";
        }

        addToCheckList(name, hash, size, url);
    }

    log( tr("Checking for obsolete addons...") );
    QString installedIndex = clientPrefix + "installed_data.json";

    JsonParser installedParser;
    if ( !installedParser.setJsonFromFile(installedIndex) )
    {
        log(versionParser.getParserError(), true);
        error( tr("Error! Can't read installed addons index.") );
        return;
    }

    auto newFiles = dataParser.getAddonsFilesInfoHashMap();
    auto oldFiles = installedParser.getAddonsFilesInfoHashMap();

    foreach ( QString oldFile, oldFiles.keys() )
    {
        if ( !newFiles.contains(oldFile) )
        {
            removeList.append(oldFile);
        }
    }

    // IV. ASSETS
    log( tr("Requesting actual assets index...") );
    if ( !versionParser.hasAssetsVersion() )
    {
        error( tr("Error! Assets are not described in version index.") );
        return;
    }

    QString assetsName = versionParser.getAssetsVesrsion() + ".json";
    QString assetsDir = settings->getAssetsDir() + "/indexes/";
    QString assetsUrl = settings->getAssetsUrl() + "indexes/";

    FileFetcher indexFetcher;
    indexFetcher.add(assetsUrl + assetsName, assetsDir + assetsName);

    connect(&indexFetcher, &FileFetcher::filesFetchResult,
            this, &UpdateDialog::assetsIndexUpdated);

    indexFetcher.fetchFiles();
}

void UpdateDialog::assetsIndexUpdated(bool result)
{
    if (!result)
    {
        error( tr("Error! Can't update assets index.") );
        return;
    }

    processAssets();
}

void UpdateDialog::processAssets()
{
    QString assetsDir = settings->getAssetsDir() + "/indexes/";
    QString assetsUrl = settings->getAssetsUrl() + "indexes/";
    QString assetsName = versionParser.getAssetsVesrsion() + ".json";

    JsonParser assetsParser;
    if ( !assetsParser.setJsonFromFile(assetsDir + assetsName) )
    {
        log(assetsParser.getParserError(), true);
        error( tr("Error! Can't read assets index.") );
        return;
    }

    log( tr("Add assets files to check list...") );
    if ( !assetsParser.hasAssetsList() )
    {
        error( tr("Error! Assets list are not described in index.") );
        return;
    }

    QString name, hash, url;
    quint64 size;

    QList<FileInfo> assets = assetsParser.getAssetsList();
    foreach (FileInfo asset, assets)
    {
        name = assetsDir + asset.name;
        hash = asset.hash;
        size = asset.size;
        url = assetsUrl + asset.name;

        addToCheckList(name, hash, size, url);
    }

    log( tr("Checking files...") );
    emit checkFiles( checkList );
}

void UpdateDialog::checkFinished()
{
    bool need_fetch = fetcher->getCount() > 0 ;
    bool need_remove = !removeList.empty();

    if (need_fetch || need_remove)
    {
        log( tr("Client checking completed, no need update.") );

        if ( need_remove )
        {
            foreach ( QString file, removeList )
            {
                log( tr("File: ") + file  + tr(" will be removed") );
            }
        }

        if ( need_fetch )
        {
            int count = fetcher->getCount();

            double size = fetcher->getFetchSize() / 1024;
            QString suffix = tr(" KiB");

            if ( size > 1024 * 1024 )
            {
                size = size / ( 1024 * 1024 );
                suffix = tr(" GiB");
            }
            else if ( size > 1024 )
            {
                size = size / 1024;
                suffix = tr(" MiB");
            }

            QString downloadCount = QString::number(count);
            QString downloadSize = QString::number(size, 'f', 2);

            log( tr("Need to download ") + downloadCount + " files.");
            log( tr("Download size ") + downloadSize + suffix);
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
    ui->clientCombo->setEnabled(false);
    ui->updateButton->setEnabled(false);

    if ( !removeList.isEmpty() )
    {
        ui->log->appendPlainText("\n # Удаление устаревших модификаций:");
        logger->append("UpdateDialog", "Removing files...\n");

        foreach (QString entry, removeList)
        {
            ui->log->appendPlainText("Удаление: " + entry);
            logger->append("UpdateDialog", "Remove " + entry + "\n");

            QFile::remove(settings->getClientPrefix(clientVersion) + "/"
                          + entry);
            ui->progressBar->setValue( int( ( float(removeList.indexOf(entry)
                                                    + 1) / removeList.size() )
                                            * 100 ) );
        }
    }

    // Replace custom files index
    QFile::remove(settings->getClientPrefix(
                      clientVersion) + "/installed_data.json");

    QDir(settings->getClientPrefix(clientVersion) + "/").mkpath(settings->getClientPrefix(
                                                                    clientVersion)
                                                                + "/");
    QFile::copy(settings->getVersionsDir() + "/" + clientVersion + "/data.json",
                settings->getClientPrefix(
                    clientVersion) + "/installed_data.json");

    if (fetcher->getFetchSize() != 0)
    {
        ui->log->appendPlainText("\n # Загрузка обновлений:");
        logger->append("UpdateDialog", "Downloading started...\n");
        fetcher->fetchFiles();
    }
    else
    {
        updateFinished();
    }
}

void UpdateDialog::updateFinished()
{
    ui->log->appendPlainText("\nОбновление выполнено!");
    logger->append("UpdateDialog", "Update completed\n");

    disconnect( ui->updateButton, SIGNAL( clicked() ), this,
                SLOT( doUpdate() ) );
    ui->updateButton->setText("Закрыть");
    connect( ui->updateButton, SIGNAL( clicked() ), this, SLOT( close() ) );
    state = CanUpdate;

    ui->clientCombo->setEnabled(true);
    ui->updateButton->setEnabled(true);
}

void UpdateDialog::addToCheckList(const QString &fileName,
                                  const QString &checkSumm, quint64 size,
                                  const QString &url)
{
    log(tr("Add to check list: ") + fileName, true);
    checkList.append( QPair< QString, QString >(fileName, checkSumm) );
    checkInfo[ fileName ] = QPair< QUrl, quint64 >(QUrl(url), size);
}

void UpdateDialog::addToFetchList(const QString &file)
{
    if (checkInfo.contains(file))
    {
        QUrl url = checkInfo[file].first;
        quint64 size = checkInfo[file].second;

        fetcher->add(url, file, size);
    }
    else
    {
        log(tr("Error: need to fetch unknown file: ") +  file);
    }
}

UpdateDialog::~UpdateDialog()
{
    checkThread.quit();
    checkThread.wait();

    delete ui;
}
