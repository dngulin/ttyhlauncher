#ifndef LAUNCHERWINDOW_H
#define LAUNCHERWINDOW_H

#include <QMainWindow>
#include <QActionGroup>

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


private slots:
    void loadPageTimeout();
    void pageLoaded(bool loaded);
    void linkClicked(const QUrl& url);

    void loadOfficial();
    void loadTtyh();

    void showSettingsDialog();
    void showChangePasswordDialog();
    void showSkinLoadDialog();
    void showUpdateManagerDialog();
    void showFeedBackDialog();
    void showAboutDialog();


private:
    Ui::LauncherWindow *ui;
    QActionGroup *newsGroup;

    void loadPage(const QUrl& url);
    void storeParameters();
};

#endif // LAUNCHERWINDOW_H
