#include "feedbackdialog.h"
#include "ui_feedbackdialog.h"

FeedbackDialog::FeedbackDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FeedbackDialog)
{
    ui->setupUi(this);
}

FeedbackDialog::~FeedbackDialog()
{
    delete ui;
}
