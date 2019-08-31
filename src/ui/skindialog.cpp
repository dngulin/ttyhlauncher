#include <QtWidgets/QFileDialog>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QMessageBox>

#include "skindialog.h"
#include "ui_skindialog.h"

SkinDialog::SkinDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SkinDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBrowse, &QToolButton::clicked, [=](bool) {
        auto title = tr("Select the skin file");
        auto dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        auto filter = tr("Skin (*.png)");
        auto path = QFileDialog::getOpenFileName(this, title, dir, filter);

        ui->pathEdit->setText(path);
    });
    connect(ui->buttonUpload, &QPushButton::clicked, [=]() {
        setEnabled(false);

        if (ui->pathEdit->text().isEmpty()) {
            showError(tr("Skin file is not selected"));
            return;
        }

        emit uploadClicked(ui->pathEdit->text(), ui->checkBoxSlim->isChecked());
    });
}

SkinDialog::~SkinDialog()
{
    delete ui;
}

void SkinDialog::showError(const QString &error) {
    setEnabled(true);
    QMessageBox::critical(this, tr("Failed to upload the skin"), error);
}

void SkinDialog::showSuccessAndClose() {
    QMessageBox::information(this, tr("Success"), tr("Skin is successfully uploaded!"));
    close();
}
