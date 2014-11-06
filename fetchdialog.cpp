#include "fetchdialog.h"
#include "ui_fetchdialog.h"

FetchDialog::FetchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FetchDialog)
{
    ui->setupUi(this);
}

FetchDialog::~FetchDialog()
{
    delete ui;
}
