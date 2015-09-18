#ifndef LAUNCHERWINDOW_H
#define LAUNCHERWINDOW_H

#include <QMainWindow>
#include <QActionGroup>

#include "settings.h"
#include "logger.h"
#include "gamerunner.h"

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
    void gameRunnerError(const QString& message);
    void gameRunnerNeedUpdate(const QString& message);
    void gameRunnerStarted();
    void gameRunnerFinished(int exitCode);

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

    GameRunner* gameRunner;

    void storeParameters();

    void showUpdateDialog(const QString message);
};

#endif // LAUNCHERWINDOW_H
