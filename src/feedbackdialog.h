#ifndef FEEDBACKDIALOG_H
#define FEEDBACKDIALOG_H

#include <QDialog>

#include "oldlogger.h"
#include "datafetcher.h"

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
    OldLogger* logger;

    DataFetcher uploader;

    void log(const QString &text);
    void msg(const QString &text);

private slots:
    void sendFeedback();
    void requestFinished(bool result);
};

#endif // FEEDBACKDIALOG_H
