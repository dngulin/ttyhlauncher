#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>

#include "settings.h"
#include "logger.h"
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
    Settings *settings;
    Logger *logger;
    FileFetcher *fetcher;

    QThread checkThread;
    HashChecker* checker;

    DataFetcher versionsFetcher;

    JsonParser versionParser, dataParser;

    QString clientVersion;

    QStringList removeList;
    QList<QPair<QString, QString> > checkList;
    QHash< QString, QPair< QUrl, quint64 > > checkInfo;

    void addToCheckList(const QString &fileName, const QString &checkSumm,
                        quint64 size, const QString &url);

    enum UpdaterState {CanCheck, Checking, CanUpdate, Updating, CanClose};

    UpdaterState state;
    void setState(UpdaterState newState);

    void log(const QString &line, bool hidden = false);
    void error(const QString &line);
    void setInteractable(bool state);

    void getUpdateSize();
    void doUpdate();

    void checkStart();
    void updateVersionIndex();
    void processClientFiles();
    void processAssets();

signals:
    void checkFiles( const QList<QPair<QString, QString> > list );

private slots:
    void clientChanged();
    void updateClicked();
    void cancelClicked();

    // Checking slots
    void versionListRequested(bool result);
    void versionIndexUpdated(bool result);
    void assetsIndexUpdated(bool result);

    void addToFetchList(const QString &file);
    void checkFinished();

    void updateFinished();
};

#endif // UPDATEDIALOG_H
