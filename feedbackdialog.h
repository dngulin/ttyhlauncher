#ifndef FEEDBACKDIALOG_H
#define FEEDBACKDIALOG_H

#include <QDialog>

namespace Ui {
class FeedbackDialog;
}

class FeedbackDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FeedbackDialog(QWidget *parent = 0);
    ~FeedbackDialog();

private:
    Ui::FeedbackDialog *ui;
};

#endif // FEEDBACKDIALOG_H
