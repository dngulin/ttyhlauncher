#ifndef SKINDIALOG_H
#define SKINDIALOG_H

#include <QDialog>

namespace Ui {
class SkinDialog;
}

class SkinDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SkinDialog(QWidget *parent = nullptr);
    ~SkinDialog() override;

    void fail(const QString &error);
    void success();

signals:
    void uploadClicked(const QString &path, bool slim);

private:
    Ui::SkinDialog *ui;
};

#endif // SKINDIALOG_H
