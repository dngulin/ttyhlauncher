#include "clonedialog.h"
#include "ui_clonedialog.h"

CloneDialog::CloneDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CloneDialog)
{
    ui->setupUi(this);
}

CloneDialog::~CloneDialog()
{
    delete ui;
}
