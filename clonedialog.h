#ifndef CLONEDIALOG_H
#define CLONEDIALOG_H

#include <QDialog>

namespace Ui {
class CloneDialog;
}

class CloneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CloneDialog(QWidget *parent = 0);
    ~CloneDialog();

private:
    Ui::CloneDialog *ui;
};

#endif // CLONEDIALOG_H
