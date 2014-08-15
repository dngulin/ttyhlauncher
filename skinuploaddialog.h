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

private slots:
    void uploadSkin();
    void openFileDialog();
};

#endif // SKINUPLOADDIALOG_H
