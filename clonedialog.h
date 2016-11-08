#ifndef CLONEDIALOG_H
#define CLONEDIALOG_H

#include <QDialog>

#include "settings.h"
#include "logger.h"
#include "datafetcher.h"

namespace Ui {
class CloneDialog;
}

class CloneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CloneDialog(QWidget *parent = 0);
    ~CloneDialog();

private:
    Ui::CloneDialog *ui;

    QProcess ttyhstore;
    DataFetcher fetcher;

    Settings* settings;
    Logger* logger;

    void requestVersions();
    void log(const QString &line, bool hidden = false);

private slots:
    void onVersionsReply(bool result);

    void clone();
    void cancel();

    void onStart();
    void onFinish(int exitCode);
    void onOutput();
    void onError(QProcess::ProcessError error);
};

#endif // CLONEDIALOG_H
