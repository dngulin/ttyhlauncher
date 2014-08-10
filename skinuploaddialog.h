#ifndef SKINUPLOADDIALOG_H
#define SKINUPLOADDIALOG_H

#include <QDialog>

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
};

#endif // SKINUPLOADDIALOG_H
