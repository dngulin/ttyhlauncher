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
    void loadPageTimeout();
    void pageLoaded(bool loaded);
    void linkClicked(const QUrl& url);

    void loadOfficial();
    void loadTtyh();

    void showSettingsDialog();
    void showSkinLoadDialog();
    void showUpdateManagerDialog();
    void showFeedBackDialog();
    void showAboutDialog();

    void playButtonClicked();


private:
    Ui::LauncherWindow *ui;
    QActionGroup *newsGroup;

    Settings* settings;
    Logger* logger;

    void loadPage(const QUrl& url);
    void storeParameters();

    bool isValidGameFile(QString fileName, QString hash);
    void runGame(QString uuid, QString acessToken, QString gameVersion);

    void unzipAllFiles(QString zipFilePath, QString extractionPath);
    void recursiveDelete(QString filePath);

    void showUpdateDialog(const QString message);
};

#endif // LAUNCHERWINDOW_H
