#include "selfupdatedialog.h"
#include "ui_selfupdatedialog.h"

#include <QStandardPaths>
typedef QStandardPaths Path;

#include "settings.h"

SelfUpdateDialog::SelfUpdateDialog(const QString &msg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SelfUpdateDialog)
{
    ui->setupUi(this);

    ui->label->setText(msg);

    QString arch = Settings::instance()->getWordSize();
    QString server = Settings::buildServer;
    QString url = server + "/build-" + arch + "-latest/ttyhlauncher.zip";

    QString temp = Path::writableLocation(Path::TempLocation);
    QString path = temp + "/ttyhlauncher.zip";

    fetcher.add(QUrl(url), path);
}

SelfUpdateDialog::~SelfUpdateDialog()
{
    delete ui;
}

void SelfUpdateDialog::updateClicked()
{
    if (complete)
    {
        // exec
    }
    else
    {
        fetcher.fetchFiles();
    }
}

void SelfUpdateDialog::cancelClicked()
{
    fetcher.cancel();
}
