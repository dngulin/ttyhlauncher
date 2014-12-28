#include "licensedialog.h"
#include "ui_licensedialog.h"

#include <QFile>
#include <QTextStream>

LicenseDialog::LicenseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LicenseDialog)
{
    ui->setupUi(this);

    logger = Logger::logger();

    // load license from resources
    QFile licenseFile(":/resources/license.txt");
    if (!licenseFile.open(QFile::ReadOnly | QFile::Text)) {
        logger->append("LicenseDialog", "Can't open license\n");
    }
    QTextStream textStream(&licenseFile);
    ui->licenseText->setPlainText(textStream.readAll());

    licenseFile.close();

    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
}

LicenseDialog::~LicenseDialog()
{
    delete ui;
}
