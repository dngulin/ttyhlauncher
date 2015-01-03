#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include "settings.h"
#include "logger.h"

namespace Ui {
class ExportDialog;
}

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(QWidget *parent = 0);
    ~ExportDialog();

private:
    Ui::ExportDialog *ui;
    Settings* settings;
    Logger* logger;

    bool copyFile(QString from, QString to);

private slots:
    void openDirDialog();
    void makeVersionList();
    void makeExport();
};

#endif // EXPORTDIALOG_H
