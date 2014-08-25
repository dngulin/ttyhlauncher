#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>

#include "logger.h"

namespace Ui {
class PasswordDialog;
}

class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordDialog(QWidget *parent = 0);
    ~PasswordDialog();

private:
    Ui::PasswordDialog *ui;

    Logger* logger;

private slots:
    void changePassword();
};

#endif // PASSWORDDIALOG_H
