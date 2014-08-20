#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>

#include "settings.h"

namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(QWidget *parent = 0);
    ~UpdateDialog();

private:
    Ui::UpdateDialog *ui;
    Settings* settings;

    QString latestver;
    QString assets;

    QNetworkAccessManager* downloadNam;
    QNetworkReply* downloadReply;
    QVector<QUrl> downloadUrls;
    QVector<QString> downloadNames;

    quint64 downloadSize;
    quint64 downloadedSize;

    bool isLatestVersionKnown(QNetworkReply* reply);

    bool checkVersionFiles();
    bool checkLibs();
    bool checkAssets();
    bool checkMods();
    void checksResult(bool allGood);

    bool downloadNow(QUrl url, QString fileName);

    void addTarget(QUrl url, QString fileName);
    void addAssetTarget(QUrl url, QString fileName, QString resName, int size);

private slots:
    void clientChanged();
    void doUpdate();

    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();

};

#endif // UPDATEDIALOG_H
