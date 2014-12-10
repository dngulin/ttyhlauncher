#ifndef CHECKOUTDIALOG_H
#define CHECKOUTDIALOG_H

#include <QDialog>
#include "settings.h"
#include "logger.h"

namespace Ui {
class CheckoutDialog;
}

class CheckoutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CheckoutDialog(QWidget *parent = 0);
    ~CheckoutDialog();

private:
    Ui::CheckoutDialog *ui;
    Settings* settings;
    Logger* logger;

    QStringList errList;

private slots:
    void makeCheckout();
    void makeVersionList();
};

#endif // CHECKOUTDIALOG_H
