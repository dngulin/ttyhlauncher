#ifndef FETCHDIALOG_H
#define FETCHDIALOG_H

#include <QDialog>
#include "settings.h"
#include "logger.h"

namespace Ui {
class FetchDialog;
}

class FetchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FetchDialog(QWidget *parent = 0);
    ~FetchDialog();

private:
    Ui::FetchDialog *ui;
    Settings* settings;
    Logger* logger;

    QStringList errList;

    void downloadFile(QString url, QString fname);

private slots:
    void makeFetch();
    void makeVersionList();
};

#endif // FETCHDIALOG_H
