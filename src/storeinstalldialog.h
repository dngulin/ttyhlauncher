#ifndef STOREINSTALLDIALOG_H
#define STOREINSTALLDIALOG_H

#include <QDialog>

#include "oldsettings.h"
#include "oldlogger.h"
#include "fileinstaller.h"
#include "fileinfo.h"

namespace Ui {
class StoreInstallDialog;
}

class StoreInstallDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StoreInstallDialog(QWidget *parent = 0);
    ~StoreInstallDialog();

private:
    Ui::StoreInstallDialog *ui;

    OldSettings* settings;
    OldLogger* logger;

    QThread installThread;
    FileInstaller* installer;

    QList<InstallInfo> installList;

    void log(const QString &line, bool hidden = false);

    void setupLocalStoreVersions();
    void getLocalPrefixVersions(const QString &prefix);
    void setupPrefixes();

    void setInteractable(bool state);

    bool installing;

    QString storeDir;
    QString clientDir;
    QString storePrefix;
    QString storeVersion;

    void prepareVersion(const QString &jarHash);
    void prepareLibararies(const QList<FileInfo> &libs);
    void prepareAddons(const QHash<QString, FileInfo> &addons);
    void prepareAssets();

signals:
    void install(const QList<InstallInfo> &list);

private slots:
    void installClicked();
    void cancelClicked();

    void installError(const InstallInfo &info);
    void installFinished();
};

#endif // STOREINSTALLDIALOG_H
