#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->labelVersion->setText(QString("%1 %2").arg(QApplication::applicationName(),
                                                   QApplication::applicationVersion()));
    adjustSize();
    setFixedSize(size());
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
