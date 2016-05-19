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
    explicit SelfUpdateDialog(const QString &msg, QWidget *parent = 0 );
    ~SelfUpdateDialog();

private:
    Ui::SelfUpdateDialog *ui;

    FileFetcher fetcher;
    bool complete;

private slots:
    void updateClicked();
    void cancelClicked();
};

#endif // SELFUPDATEDIALOG_H
