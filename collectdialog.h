#ifndef COLLECTDIALOG_H
#define COLLECTDIALOG_H

#include <QDialog>
#include <QProcess>

#include "settings.h"
#include "logger.h"

namespace Ui {
class CollectDialog;
}

class CollectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CollectDialog(QWidget *parent = 0);
    ~CollectDialog();

private:
    Settings *settings;
    Logger   *logger;

    QProcess ttyhstore;

    void log(const QString &line, bool hidden = false);

private slots:
    void collect();
    void cancel();

    void onStart();
    void onFinish(int exitCode);
    void onOutput();
    void onError(QProcess::ProcessError error);

private:
    Ui::CollectDialog *ui;
};

#endif // COLLECTDIALOG_H
