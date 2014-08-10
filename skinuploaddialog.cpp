#include "skinuploaddialog.h"
#include "ui_skinuploaddialog.h"

SkinUploadDialog::SkinUploadDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SkinUploadDialog)
{
    ui->setupUi(this);
}

SkinUploadDialog::~SkinUploadDialog()
{
    delete ui;
}
