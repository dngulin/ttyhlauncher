#ifndef SELFUPDATEDIALOG_H
#define SELFUPDATEDIALOG_H

#include <QDialog>

#include "filefetcher.h"

namespace Ui {
class SelfUpdateDialog;
}

class SelfUpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelfUpdateDialog(const QString &text, QWidget *parent = 0);
    ~SelfUpdateDialog();

private:
    Ui::SelfUpdateDialog *ui;

    FileFetcher fetcher;
    bool complete;

    void log(const QString &text);
    void msg(const QString &text);

private slots:
    void updateClicked();
    void cancelClicked();

    void fetchSizeFinished(bool result);
    void downloadFinished(bool result);
};

#endif // SELFUPDATEDIALOG_H
