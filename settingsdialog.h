#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QNetworkReply>
#include <QNetworkAccessManager>

#include "settings.h"

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
    void makeLocalVersionList(QString reason);


private slots:
    void saveSettings();
    void loadSettings();
    void openFileDialog();
    void openClientDirectory();
    void loadVersionList();
    void makeVersionList(QNetworkReply* reply);
};

#endif // SETTINGSDIALOG_H
