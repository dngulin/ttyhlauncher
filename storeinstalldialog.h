#ifndef STOREINSTALLDIALOG_H
#define STOREINSTALLDIALOG_H

#include <QDialog>

#include "settings.h"
#include "logger.h"
#include "fileinstaller.h"

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

    Settings* settings;
    Logger* logger;

    QThread installThread;
    FileInstaller* installer;

    QList<InstallInfo> installList;

    void log(const QString &line, bool hidden = false);

    void setupLocalStoreVersions();
    void getLocalPrefixVersions(const QString &prefix);
    void setupPrefixes();

    void setInteractable(bool state);

signals:
    void install(const QList<InstallInfo> &list);

private slots:
    void installClicked();
    void cancelClicked();

    void installError(const InstallInfo &info);
    void installFinished();
};

#endif // STOREINSTALLDIALOG_H
