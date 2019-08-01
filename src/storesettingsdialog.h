#ifndef STORESETTINGSDIALOG_H
#define STORESETTINGSDIALOG_H

#include <QDialog>

#include "oldsettings.h"
#include "oldlogger.h"

namespace Ui {
class StoreSettingsDialog;
}

class StoreSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StoreSettingsDialog(QWidget *parent = 0);
    ~StoreSettingsDialog();

private:
    Ui::StoreSettingsDialog *ui;

    OldSettings* settings;
    OldLogger* logger;

    void log(const QString &text);
    void loadSettings();

    void openSelectExeDialog();
    void openSelectDirDialog();

private slots:
    void saveSettings();
};

#endif // STORESETTINGSDIALOG_H
