#ifndef SKINUPLOADDIALOG_H
#define SKINUPLOADDIALOG_H

#include <QDialog>

#include "logger.h"
#include "datafetcher.h"

namespace Ui {
class SkinUploadDialog;
}

class SkinUploadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SkinUploadDialog(QWidget *parent = 0);
    ~SkinUploadDialog();

private:
    Ui::SkinUploadDialog *ui;

    Logger *logger;
    DataFetcher uploader;

    void log(const QString &text);
    void msg(const QString &text);

private slots:
    void uploadSkin();
    void requestFinished(bool result);
    void openFileDialog();
};

#endif // SKINUPLOADDIALOG_H
