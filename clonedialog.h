#ifndef CLONEDIALOG_H
#define CLONEDIALOG_H

#include <QDialog>
#include "settings.h"
#include "logger.h"

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
    Settings* settings;
    Logger* logger;

    bool loadVersionList();

private slots:
    void makeClone();
};

#endif // CLONEDIALOG_H
