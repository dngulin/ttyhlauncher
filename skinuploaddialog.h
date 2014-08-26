#ifndef SKINUPLOADDIALOG_H
#define SKINUPLOADDIALOG_H

#include <QDialog>

#include "logger.h"

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

    Logger* logger;

private slots:
    void uploadSkin();
    void openFileDialog();
};

#endif // SKINUPLOADDIALOG_H
