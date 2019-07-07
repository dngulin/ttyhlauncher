#include "licensedialog.h"
#include "../data/ui_licensedialog.h"

#include <QFile>
#include <QTextStream>

LicenseDialog::LicenseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LicenseDialog)
{
    ui->setupUi(this);

    logger = Logger::logger();

    QFile licenseFile(":/resources/license.txt");
    if ( !licenseFile.open(QFile::ReadOnly | QFile::Text) )
    {
        logger->appendLine( tr("LicenseDialog"), tr("Can't open license.") );
    }
    else
    {
        QTextStream textStream(&licenseFile);
        ui->licenseText->setPlainText( textStream.readAll() );

        licenseFile.close();
    }

    connect(ui->closeButton, &QPushButton::clicked, this,
            &LicenseDialog::close);
}

LicenseDialog::~LicenseDialog()
{
    delete ui;
}
