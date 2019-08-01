#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QtNetwork>

#include "oldsettings.h"
#include "oldlogger.h"
#include "datafetcher.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

private:
    Ui::SettingsDialog *ui;

    DataFetcher fetcher;

    OldSettings *settings;
    OldLogger *logger;

    void msg(const QString &text);
    void log(const QString &text);

    void appendVersionList(const QString &reason);
    void logCurrentSettings();

    bool isVersionInstalled(const QString &name);

private slots:
    void saveSettings();
    void loadSettings();
    void chooseJavaPath();
    void chooseJavaKeystore();
    void openClientDirectory();
    void loadVersionList();
    void makeVersionList(bool result);
};

#endif // SETTINGSDIALOG_H
