#ifndef CLONEDIALOG_H
#define CLONEDIALOG_H

#include <QDialog>

#include "oldsettings.h"
#include "oldlogger.h"
#include "datafetcher.h"

namespace Ui {
class StoreManageDialog;
}

class StoreManageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StoreManageDialog(QWidget *parent = 0);
    ~StoreManageDialog();

private:
    Ui::StoreManageDialog *ui;

    QProcess* ttyhstore;
    DataFetcher fetcher;

    OldSettings* settings;
    OldLogger* logger;

    void requestVersions();
    void log(const QString &line, bool hidden = false);

    void setControlsEnabled(bool state);

private slots:
    void onCommandSwitched(int id);
    void onVersionsReply(bool result);

    void runCommand();
    void cancel();

    void onStart();
    void onFinish(int exitCode);
    void onOutput();
    void onError(QProcess::ProcessError error);
};

#endif // CLONEDIALOG_H
