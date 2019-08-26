#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void setLocked(bool locked);
    void appendLog(const QString &msg);

    void setUserName(const QString &username);
    QString getUserName() const;

    void setPassword(const QString &password);
    QString getPassword() const;

    void setProfiles(const QStringList &profiles, const QString &selected);
    QString getSelectedProfile() const;

    void setSavePassword(bool savePassword);
    bool isSavePassword() const;

    void setHideOnRun(bool hideOnRun);
    bool isHideOnRun() const;

    void setOnline(bool online);
    bool isOnline() const;

    void showTask(const QString &taskName);
    void setTaskRange(int min, int max);
    void setTaskTarget(int id, const QString &name);
    void hideTask();

    void showMessage(const QString &message);
    void showError(const QString &error);
    bool askForDownloads(int count, quint64 size);
    bool askForProfileDeletion(const QString &profileName);

signals:
    void onlineModeSwitched(bool online);
    void playClicked();
    void uploadSkinClicked();
    void taskCancelled();

    void profileEditClicked();
    void profileCreateClicked();
    void profileRemoveClicked();

    void closed();
    void resized(const QSize &size);

private:
    Ui::MainWindow *ui;
    QAction *actionCreateProfile;
    QAction *actionEditProfile;
    QAction *actionRemoveProfile;

    static QString getSizeString(quint64 size);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

#endif // MAINWINDOW_H
