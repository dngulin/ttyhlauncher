#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QtNetwork>

#include "settings.h"
#include "logger.h"

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
    QNetworkAccessManager* nam;

    Settings* settings;
    Logger* logger;

    void appendVersionList(QString reason);


private slots:
    void toggleFullscreen(bool b);
    void saveSettings();
    void loadSettings();
    void openFileDialog();
    void openClientDirectory();
    void loadVersionList();
    void makeVersionList(QNetworkReply* reply);
};

#endif // SETTINGSDIALOG_H
