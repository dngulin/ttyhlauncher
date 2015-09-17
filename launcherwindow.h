#ifndef LAUNCHERWINDOW_H
#define LAUNCHERWINDOW_H

#include <QMainWindow>
#include <QActionGroup>

#include "settings.h"
#include "logger.h"

namespace Ui {
class LauncherWindow;
}

class LauncherWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit LauncherWindow(QWidget *parent = 0);
    ~LauncherWindow();
    void closeEvent (QCloseEvent* event);
    void keyPressEvent(QKeyEvent* pe);


private slots:
    void linkClicked(const QUrl& url);

    void showSettingsDialog();
    void showSkinLoadDialog();
    void showUpdateManagerDialog();
    void showFeedBackDialog();
    void showAboutDialog();

    void offlineModeChanged();
    void hideWindowModeChanged();

    void freezeInterface();
    void unfreezeInterface();

    void playButtonClicked();
    void gameRunError(QProcess::ProcessError error);
    void gameRunSuccess();
    void gameRunReadyOutput();
    void gameRunFinished(int exitCode);

    void switchBuilderMenuVisibility();

    void showCloneDialog();
    void showFetchDialog();
    void showCheckoutDialog();
    void showExportDialog();

    void showError(const QString & title, const QString & message);


private:
    Ui::LauncherWindow *ui;
    QActionGroup *newsGroup;

    Settings* settings;
    Logger* logger;

    QProcess* minecraft;

    void storeParameters();

    bool isValidGameFile(QString fileName, QString hash);
    void runGame(QString uuid, QString accessToken, QString gameVersion);

    void unzipAllFiles(QString zipFilePath, QString extractionPath);
    void recursiveDelete(QString filePath);

    void showUpdateDialog(const QString message);
};

#endif // LAUNCHERWINDOW_H
