#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "settings.h"

#include <QFileDialog>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    Settings* settings = Settings::instance();

    // Setup client combobox
    ui->clientCombo->addItems(settings->getClientsNames());
    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
    connect(ui->clientCombo, SIGNAL(activated(int)), settings, SLOT(saveActiveClientId(int)));
    connect(ui->clientCombo, SIGNAL(activated(int)), this, SLOT(loadSettings()));
    emit ui->clientCombo->activated(ui->clientCombo->currentIndex());

    connect(ui->javapathButton, SIGNAL(clicked()), this, SLOT(openFileDialog()));

    connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveSettings()));

}

void SettingsDialog::saveSettings() {
    Settings* settings = Settings::instance();

    int vid = ui->versionCombo->currentIndex();
    QString vstrid = ui->versionCombo->itemData(vid).toString();
    settings->saveClientVersion(vstrid);

    settings->saveClientJavaState(ui->javapathBox->isChecked());
    settings->saveClientJava(ui->javapathEdit->text());
    settings->saveClientJavaArgsState(ui->argsBox->isChecked());
    settings->saveClientJavaArgs(ui->argsEdit->text());
}

void SettingsDialog::loadSettings() {
    Settings* settings = Settings::instance();

    // Setup version combobox
    ui->versionCombo->addItem("Актуальная", QString("latest"));
    // FIXME: Need to get version list from internet

    QString strvid = settings->loadClientVersion();
    bool found = false;
    for (int i = 0; i < ui->versionCombo->count(); i++) {
        if (ui->versionCombo->itemData(i) == strvid) {
            ui->versionCombo->setCurrentIndex(i);
            found = true;
            break;
        }
    }

    if (!found) {
        ui->versionCombo->addItem(strvid, strvid);
        ui->versionCombo->setCurrentText(strvid);
        ui->stateEdit->setText("Неизвестная версия");
    }

    // Setup settings
    ui->javapathBox->setChecked(settings->loadClientJavaState());
    ui->javapathEdit->setText(settings->loadClientJava());
    ui->argsBox->setChecked(settings->loadClientJavaArgsState());
    ui->argsEdit->setText(settings->loadClientJavaArgs());
}

void SettingsDialog::openFileDialog() {
    QString javapath = QFileDialog::getOpenFileName(this, "Выберите исполняемый файл java", "", "");
    ui->javapathEdit->setText(javapath);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
