#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>

#include "oldsettings.h"
#include "oldlogger.h"
#include "filefetcher.h"
#include "datafetcher.h"
#include "jsonparser.h"
#include "hashchecker.h"

namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(QString displayMessage, QWidget *parent = 0);
    ~UpdateDialog();

private:
    Ui::UpdateDialog *ui;
    OldSettings *settings;
    OldLogger *logger;

    QThread checkThread;
    HashChecker *checker;

    DataFetcher dataFetcher;
    FileFetcher fileFetcher;

    JsonParser versionParser, dataParser, assetsParser;

    QString clientVersion;

    QStringList removeList;
    QList<FileInfo> checkList;

    enum UpdaterState {CanCheck, Checking, CanUpdate, Updating, CanClose};

    UpdaterState state;
    void setState(UpdaterState newState);

    void resetUpdateData();

    void log(const QString &line, bool hidden = false);
    void error(const QString &line);
    void setInteractable(bool state);

    void doCheck();
    void requestVersionIndex();
    void requestDataIndex();
    void requestAssetsIndex();
    void processIndexesData();

    void doUpdate();

signals:
    void checkFiles(const QList<FileInfo> list, bool stopOnBad);

private slots:
    void clientChanged();
    void updateClicked();
    void cancelClicked();

    void versionListReceived(bool result);
    void versionIndexReceived(bool result);
    void dataIndexReceived(bool result);
    void assetsIndexReceived(bool result);

    void addToFetchList(const FileInfo fileInfo);
    void checkFinished();

    void updateComplete(bool result);
};

#endif // UPDATEDIALOG_H
