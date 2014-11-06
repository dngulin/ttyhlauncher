#ifndef FETCHDIALOG_H
#define FETCHDIALOG_H

#include <QDialog>

namespace Ui {
class FetchDialog;
}

class FetchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FetchDialog(QWidget *parent = 0);
    ~FetchDialog();

private:
    Ui::FetchDialog *ui;
};

#endif // FETCHDIALOG_H
