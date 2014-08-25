#ifndef FEEDBACKDIALOG_H
#define FEEDBACKDIALOG_H

#include <QDialog>

#include "logger.h"

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
    Logger* logger;

private slots:
    void sendFeedback();
};

#endif // FEEDBACKDIALOG_H
