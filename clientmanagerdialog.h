#ifndef CLIENTMANAGERDIALOG_H
#define CLIENTMANAGERDIALOG_H

#include <QDialog>

namespace Ui {
class ClientManagerDialog;
}

class ClientManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ClientManagerDialog(QWidget *parent = 0);
    ~ClientManagerDialog();

private:
    Ui::ClientManagerDialog *ui;
};

#endif // CLIENTMANAGERDIALOG_H
